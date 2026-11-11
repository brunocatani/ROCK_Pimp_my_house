#pragma once
#include <cstdint>
#include <initializer_list>
#include <ROCK/Grab.h>
namespace pimp_my_house {
    // A click belongs to the bipod on an equipped weapon, so every weapon is
    // placed by holding it instead; ordinary objects keep the plain click.
    inline constexpr float kWeaponHoldSeconds = 1.0f;

    struct Candidate
    {
        std::uint32_t formId{};
        bool hold{};
        bool equipped{}; // ROCK's bipod keeps this press: it is never reserved.
    };

    // A hand's click places what that hand holds, so one object in each hand
    // gives two independent clicks; an object held in both hands answers
    // either. The equipped weapon answers only its firing hand's click, and
    // only while that hand holds nothing.
    inline Candidate candidate(const rock::api::grab::HeldPlacementState& state, bool left) noexcept {
        using Flag=rock::api::grab::HeldPlacementFlag;
        const auto has=[](const rock::api::grab::HeldPlacementHand& hand,Flag flag) {
            return (hand.flags&static_cast<std::uint32_t>(flag))!=0;
        };
        const auto& hand=left ? state.left : state.right;
        const auto& other=left ? state.right : state.left;
        if (hand.formId) {
            const bool shared=other.formId==hand.formId;
            if (!has(hand,Flag::LooseObject) || (shared && !has(other,Flag::LooseObject))) return {};
            if (!has(hand,Flag::FixedSurfaceContact) && !(shared && has(other,Flag::FixedSurfaceContact))) return {};
            return {hand.formId,has(hand,Flag::Weapon),false};
        }
        const auto& weapon=state.equippedWeapon;
        if (weapon.formId && state.equippedWeaponHand==(left ? rock::api::Hand::Left : rock::api::Hand::Right) &&
            has(weapon,Flag::FixedSurfaceContact))
            return {weapon.formId,true,true};
        return {};
    }

    struct ClickState
    {
        bool armed{};
        bool draining{};
        bool reserved{};
        std::uint32_t request{};
        std::uint32_t holding{};
        float heldSeconds{};

        void update(const Candidate& candidate, bool available,
            bool held, bool pressed, std::uint32_t ageMilliseconds, float deltaSeconds) noexcept
        {
            request = 0;
            reserved = false;
            if (!available || ageMilliseconds > 100) {
                armed = false;
                draining = false;
                holding = 0;
                return;
            }
            // Retain ownership through the release of an accepted click even
            // after the object is released.
            reserved = draining || (candidate.formId != 0 && !candidate.equipped);
            const bool press = armed && pressed && candidate.formId != 0 && !draining;
            if (!held) {
                if (press && !candidate.hold) request = candidate.formId;
                armed = true;
                draining = false;
                holding = 0;
                return;
            }
            if (press && !candidate.hold) {
                request = candidate.formId;
                draining = true;
            } else if (press) {
                holding = candidate.formId;
                heldSeconds = 0.0f;
            } else if (holding) {
                if (deltaSeconds > 0.0f) heldSeconds += deltaSeconds;
                // The hold had to start on a surface. Contact may flicker while
                // the hand trembles, so a finished hold waits for it to return.
                if (heldSeconds >= kWeaponHoldSeconds && candidate.hold && candidate.formId == holding) {
                    request = holding;
                    draining = true;
                    holding = 0;
                }
            }
            armed = false;
        }
    };
}
