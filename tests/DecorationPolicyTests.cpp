#include "DecorationPolicy.h"
#include <cassert>
int main() {
 using namespace pimp_my_house;
 using namespace rock::api::grab;
 constexpr bool right=false, left=true;
 HeldPlacementState held;
 assert(candidate(held,right).formId==0 && candidate(held,left).formId==0);
 // Each click places only its own hand's object: a left object never answers the right click.
 held.left={18,3}; assert(candidate(held,left).formId==18 && candidate(held,right).formId==0);
 held.right={17,1}; assert(candidate(held,right).formId==0 && candidate(held,left).formId==18);
 held.right={17,3}; assert(candidate(held,right).formId==17 && candidate(held,left).formId==18);
 // One object in both hands touches through either hand and answers either click.
 held.left={17,1}; assert(candidate(held,left).formId==17 && candidate(held,right).formId==17);
 held.right={17,0}; assert(candidate(held,left).formId==0);

 // A weapon in the hands or the equipped one is held. The equipped weapon
 // answers only its firing hand's click while that hand is empty.
 held={};held.right={17,7};assert(candidate(held,right).hold && !candidate(held,right).equipped);
 held.equippedWeapon={30,6};held.equippedWeaponHand=rock::api::Hand::Right;assert(candidate(held,right).formId==17);
 held.right={};assert(candidate(held,right).formId==30 && candidate(held,right).hold && candidate(held,right).equipped);
 assert(candidate(held,left).formId==0);
 held.equippedWeaponHand=rock::api::Hand::Left;assert(candidate(held,left).formId==30 && candidate(held,right).formId==0);
 held.equippedWeapon.flags=4;assert(candidate(held,left).formId==0);

 const Candidate object{17,false,false};
 const Candidate none{};
 ClickState state;
 state.update(object,true,false,false,0,0.1f);assert(state.reserved && !state.request);
 state.update(object,true,true,true,0,0.1f);assert(state.request==17 && state.draining);
 state.update(object,true,true,false,0,0.1f);assert(!state.request && state.reserved);
 state.update(none,true,true,false,0,0.1f);assert(!state.request && state.reserved);
 state.update(none,true,false,false,0,0.1f);assert(state.reserved && !state.draining);
 state.update(none,true,false,false,0,0.1f);assert(!state.reserved);
 state.update(none,true,true,true,0,0.1f);state.update(object,true,true,false,0,0.1f);assert(!state.request);
 state.update(object,true,false,false,0,0.1f);state.update(object,true,false,true,0,0.1f);assert(state.request==17);
 state.update(object,false,true,true,0,0.1f);state.update(object,true,true,true,0,0.1f);assert(!state.request);
 state.update(object,true,false,false,0,0.1f);state.update(object,true,true,true,101,0.1f);assert(!state.request && !state.reserved);

 const Candidate equipped{30,true,true};
 const Candidate looseWeapon{31,true,false};
 const auto press=[&](const Candidate& target) {
  state={};
  state.update(target,true,false,false,0,0.1f);
  state.update(target,true,true,true,0,0.1f);
 };
 // A tap never places a weapon, including one pressed and released within a frame.
 press(equipped);assert(!state.request && !state.reserved);
 state.update(equipped,true,false,false,0,0.1f);assert(!state.request && !state.holding);
 state={};state.update(equipped,true,false,false,0,0.1f);state.update(equipped,true,false,true,0,0.1f);assert(!state.request);

 // The hold completes on elapsed time, whatever the frame rate. Only a loose
 // weapon reserves the click: the equipped weapon's press stays with the bipod.
 press(equipped);
 for (int i=0;i<9;++i) { state.update(equipped,true,true,false,0,0.1f);assert(!state.request && !state.reserved); }
 state.update(equipped,true,true,false,0,0.11f);assert(state.request==30 && !state.reserved);
 state.update(none,true,true,false,0,5.0f);assert(!state.request && state.reserved);
 press(looseWeapon);assert(state.reserved && !state.request);
 state.update(looseWeapon,true,true,false,0,2.0f);assert(state.request==31);

 // A finished hold waits for the surface; a press off the surface never starts one.
 press(equipped);
 state.update(none,true,true,false,0,2.0f);assert(!state.request);
 state.update(equipped,true,true,false,0,0.0f);assert(state.request==30);
 press(none);state.update(equipped,true,true,false,0,2.0f);assert(!state.request && !state.holding);

 // Letting go or losing the input ends the hold, and an unmeasurable frame adds no time.
 press(equipped);state.update(equipped,true,false,false,0,0.5f);state.update(equipped,true,true,false,0,2.0f);assert(!state.request);
 press(equipped);state.update(equipped,false,true,false,0,0.5f);state.update(equipped,true,true,false,0,2.0f);assert(!state.request);
 press(equipped);state.update(equipped,true,true,false,0,0.0f);assert(!state.request && state.holding==30);
}
