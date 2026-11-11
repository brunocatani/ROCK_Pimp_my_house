// Before <Windows.h>: it includes CommonLibF4's RE headers.
#include <RPS/Runtime/RequiredUpdate.h>
#include <F4SE/F4SE.h>
#include <REL/Relocation.h>
#include <Windows.h>
#include <ShlObj.h>
#include <ROCK/Animation.h>
#include <ROCK/Client.h>
#include <ROCK/Grab.h>
#include "DecorationPolicy.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace {
    constexpr const char* kName="ROCK_Pimp_my_house";
    // Process-lifetime plugin. Animation owns the callback registration; all API
    // writes and this gesture state are confined to its BeforeRock frame phase.
    rock::api::Client client;
    const rock::api::grab::Api* grabApi{};
    const rock::api::animation::Api* animationApi{};
    std::uint64_t callbackToken{}, pendingToken{};
    bool callbackFailed{};
    rock::api::Sample previousSample{};
    // One gesture per stick click: [0] right, [1] left.
    std::array<pimp_my_house::ClickState,2> clicks;
    std::shared_ptr<spdlog::logger> pluginLog;

    void update(const rock::api::animation::AnimationPhaseContext* phase) {
        using namespace rock::api;
        using namespace rock::api::grab;
        using animation::AnimationPhaseContextFlag;
        if (!phase || phase->phase!=animation::AnimationPhase::BeforeRock) return;
        const auto blocked=static_cast<std::uint32_t>(AnimationPhaseContextFlag::MenuBlocking)|
            static_cast<std::uint32_t>(AnimationPhaseContextFlag::ConfigBlocking);
        const auto ready=static_cast<std::uint32_t>(AnimationPhaseContextFlag::ProviderReady)|
            static_cast<std::uint32_t>(AnimationPhaseContextFlag::SkeletonReady);
        HeldPlacementState state;
        if ((phase->flags&blocked) || (phase->flags&ready)!=ready ||
            grabApi->getHeldPlacementState(client.owner(),&state)!=Status::Ok) {
            clicks={}; pendingToken=0;
            (void)grabApi->clearHeldPlacementIntent(client.owner());
            return;
        }
        if (state.sample.worldGeneration!=previousSample.worldGeneration ||
            state.sample.skeletonGeneration!=previousSample.skeletonGeneration ||
            state.sample.providerGeneration!=previousSample.providerGeneration) { clicks={};pendingToken=0; }
        previousSample=state.sample;
        if (pendingToken && state.lastRequestToken==pendingToken &&
            (state.lastResult==PlacementResult::Anchored || state.lastResult==PlacementResult::Rejected)) {
            pluginLog->info("Placement result ref={:08X} anchored={}",state.lastRequestFormId,state.lastResult==PlacementResult::Anchored);
            pendingToken=0;
        }
        const std::array eligible{pimp_my_house::candidate(state,false),pimp_my_house::candidate(state,true)};
        const std::array buttons{&state.rightButton,&state.leftButton};
        for (std::size_t i=0;i<clicks.size();++i)
            clicks[i].update(eligible[i],buttons[i]->available!=0,buttons[i]->held!=0,buttons[i]->pressed!=0,
                buttons[i]->ageMilliseconds,phase->deltaSeconds);
        HeldPlacementIntent intent;
        intent.frameToken=state.frameToken; intent.sample=state.sample;
        // One placement per update; the right click goes first when both land together.
        const std::size_t hand=clicks[0].request ? 0 : 1;
        const bool request=clicks[hand].request && !pendingToken;
        const char* handName=hand==0 ? "right" : "left";
        if (request) intent.formId=clicks[hand].request;
        if (clicks[0].reserved) intent.flags|=static_cast<std::uint32_t>(PlacementIntentFlag::ReserveRightClick);
        if (clicks[1].reserved) intent.flags|=static_cast<std::uint32_t>(PlacementIntentFlag::ReserveLeftClick);
        if (state.equippedWeapon.formId) intent.flags|=static_cast<std::uint32_t>(PlacementIntentFlag::TimeEquippedWeaponHold);
        if (request) intent.flags|=static_cast<std::uint32_t>(eligible[hand].equipped ? PlacementIntentFlag::AnchorEquippedWeapon : PlacementIntentFlag::Anchor);
        const auto status=grabApi->submitHeldPlacementIntent(client.owner(),&intent);
        if (status!=Status::RequestQueued) {
            if (request) pluginLog->warn("Placement request rejected ref={:08X} hand={} status={}",intent.formId,handName,static_cast<unsigned>(status));
            clicks={};return;
        }
        if (request) {
            pendingToken=state.frameToken;
            pluginLog->info("Placement requested ref={:08X} hand={}",intent.formId,handName);
        }
    }

    void ROCK_CALL onPhase(const rock::api::animation::AnimationPhaseContext* phase,void*) noexcept {
        if (callbackFailed) return;
        try { update(phase); }
        catch (...) {
            callbackFailed=true;
            clicks={}; pendingToken=0;
            (void)grabApi->clearHeldPlacementIntent(client.owner());
            try { pluginLog->error("Placement callback failed; decoration disabled for this session"); }
            catch (...) { OutputDebugStringA("ROCK_Pimp_my_house: placement callback failed\n"); }
        }
    }

    // A loaded ROCK that does not serve the API this add-on was built for stops
    // the game at the main menu (RPS/Runtime/RequiredUpdate.h); a missing ROCK
    // does not. The add-on requests the exact versions its ROCK headers
    // declare, so every version refusal is a real mismatch.
    bool isVersionRefusal(const rock::api::Status status) {
        using rock::api::Status;
        return status==Status::UnknownInterface || status==Status::UnsupportedMajor || status==Status::UnsupportedMinor ||
            status==Status::TableTooSmall || status==Status::InvalidSize;
    }

    void reportRockMismatch(const char* reason) {
        pluginLog->critical("ROCK Pimp my House does not work with the loaded ROCK ({}): the game stops at the main menu until they match",reason);
        RPS::Runtime::RequiredUpdate::report(L"ROCK Pimp my House does not work with the installed ROCK: they were "
            L"built for different ROCK versions. Install matching versions of ROCK and ROCK Pimp my House.");
    }

    void connect() {
        using rock::api::Status;
        if (client.owner()) return;
        if (!RPS::Runtime::RequiredUpdate::watchMainMenu(L"ROCK Pimp my House: update required"))
            pluginLog->error("The UI is unavailable; a ROCK version mismatch cannot stop the game at the main menu");
        const auto module=GetModuleHandleA("ROCK.dll");
        if (!module) { pluginLog->error("ROCK.dll is not loaded; decoration stays off"); return; }
        const auto query=reinterpret_cast<rock::api::QueryInterfaceV1>(GetProcAddress(module,rock::api::kQueryExportName));
        if (!query) { reportRockMismatch("no ROCKAPI_QueryInterfaceV1 export"); return; }
        auto status=client.connect(query,kName);
        using GrabPermission=rock::api::grab::Permission;
        if (status==Status::Ok) status=client.acquire(static_cast<std::uint32_t>(rock::api::animation::Permission::Callbacks),animationApi);
        if (status==Status::Ok) status=client.acquire(static_cast<std::uint32_t>(GrabPermission::Read)|static_cast<std::uint32_t>(GrabPermission::Write),grabApi);
        rock::api::animation::PhaseCallbackRegistration phases{};
        phases.phaseMask=rock::api::animation::phaseBit(rock::api::animation::AnimationPhase::BeforeRock);
        phases.callback=onPhase;
        if (status==Status::Ok) status=animationApi->registerAnimationPhaseCallback(client.owner(),&phases,&callbackToken);
        if (status!=Status::Ok) {
            pluginLog->error("ROCK connection failed: status={}; requires ROCK Animation and Grab 2.0",static_cast<unsigned>(status));
            if (isVersionRefusal(status)) reportRockMismatch("an interface refused");
            const auto closed=client.close();
            if (closed!=Status::Ok) pluginLog->error("ROCK owner cleanup failed: {}",static_cast<unsigned>(closed));
            return;
        }
        pluginLog->info("Decoration active: the stick click of the hand holding an object touching a surface places it, {:.0f} second hold on a weapon; ordinary grab/pull unlocks",pimp_my_house::kWeaponHoldSeconds);
    }

    // F4SE::log::log_directory() names the save folder, which F4SE only knows
    // after Init in Load; Query runs first and only accepts Fallout 4 VR.
    std::optional<std::filesystem::path> logPath() {
        PWSTR raw{};
        const auto result=SHGetKnownFolderPath(FOLDERID_Documents,KF_FLAG_DEFAULT,nullptr,&raw);
        const std::unique_ptr<wchar_t,decltype(&CoTaskMemFree)> documents(raw,&CoTaskMemFree);
        if (FAILED(result) || !documents) return std::nullopt;
        return std::filesystem::path(documents.get())/"My Games"/"Fallout4VR"/"F4SE"/(std::string(kName)+".log");
    }

    void onMessage(F4SE::MessagingInterface::Message* message) noexcept {
        if (!message) return;
        try {
            if (message->type==F4SE::MessagingInterface::kGameLoaded || message->type==F4SE::MessagingInterface::kGameDataReady)
                connect();
        } catch (...) { OutputDebugStringA("ROCK_Pimp_my_house: plugin message handling failed\n"); }
    }
}

extern "C" __declspec(dllexport) bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* query,F4SE::PluginInfo* info) noexcept {
    try {
        if (!query || !info) return false;
        const auto path=logPath();
        if (!path) return false;
        pluginLog=std::make_shared<spdlog::logger>(kName,std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path->string(),2*1024*1024,2,true));
        pluginLog->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] %v");pluginLog->flush_on(spdlog::level::info);
        info->infoVersion=F4SE::PluginInfo::kVersion;info->name=kName;info->version=1;
        if (query->IsEditor() || !REL::Module::IsVR() || REL::Module::get().version()!=F4SE::RUNTIME_VR_1_2_72) return false;
        pluginLog->info("Query passed for Fallout4VR.exe 1.2.72; version 0.1.0");
        return true;
    } catch (...) { return false; }
}
extern "C" __declspec(dllexport) bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* load) noexcept {
    try {
        if (!load) return false;
        F4SE::Init(load,false);
        const auto* messaging=F4SE::GetMessagingInterface();
        if (!messaging || !messaging->RegisterListener(onMessage)) return false;
        pluginLog->info("Load complete; waiting for ROCK");
        return true;
    } catch (...) { return false; }
}
