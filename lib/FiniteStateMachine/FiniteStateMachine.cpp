#include "FiniteStateMachine.h"

//FINITE STATE
State::State( void (*updateFunction)() ){
	id = 0;
	userEnter = 0;
	userUpdate = updateFunction;
	userExit = 0;
}

State::State( byte stateID, void (*updateFunction)() ){
	id = stateID;
	userEnter = 0;
	userUpdate = updateFunction;
	userExit = 0;
}

State::State( byte stateID ){
	id = stateID;
	userEnter = 0;
	userUpdate = 0;
	userExit = 0;
}

State::State( void (*enterFunction)(), void (*updateFunction)(), void (*exitFunction)() ){
	id = 0;
	userEnter = enterFunction;
	userUpdate = updateFunction;
	userExit = exitFunction;
}

State::State( byte stateID, void (*enterFunction)(), void (*updateFunction)(), void (*exitFunction)() ){
	id = stateID;
	userEnter = enterFunction;
	userUpdate = updateFunction;
	userExit = exitFunction;
}

byte State::getID(){
	return id;
}

//what to do when entering this state
void State::enter(){
	if (userEnter){
		userEnter();
	}
}

//what to do when this state updates
void State::update(){
	if (userUpdate){
		userUpdate();
	}
}

//what to do when exiting this state
void State::exit(){
	if (userExit){
		userExit();
	}
}

State& State::getNext() {
	return *nextState;
}

void State::setNext(State& next) {
	nextState = &next;
}

bool State::operator==(State& rhs)  {
	return id == rhs.getID();
}

bool State::operator!=(State& rhs)  {
	return id != rhs.getID();
}
//END FINITE STATE


//FINITE STATE MACHINE
FiniteStateMachine::FiniteStateMachine(State& current){
	needToTriggerEnter = true;
	previousState = currentState = nextState = &current;
	stateChangeTime = 0;
}

FiniteStateMachine& FiniteStateMachine::update() {
	//simulate a transition to the first state
	//this only happens the first time update is called
	if (needToTriggerEnter) {
		currentState->enter();
		needToTriggerEnter = false;
	} else {
		if (currentState != nextState){
			//immediateTransitionTo(*nextState);
			transitionTo(*nextState);
		}
		currentState->update();
	}
	return *this;
}
/*
FiniteStateMachine& FiniteStateMachine::transitionTo(State& state){
	previousState = currentState;
	nextState = &state;
	stateChangeTime = millis();
	return *this;
}
*/
//FiniteStateMachine& FiniteStateMachine::immediateTransitionTo(State& state){
FiniteStateMachine& FiniteStateMachine::transitionTo(State& state){
	previousState = currentState;
	currentState->exit();
	currentState = nextState = &state;
	currentState->enter();
	stateChangeTime = millis();
	return *this;
}

void FiniteStateMachine::tick() {
	index++;
	if (index==maxTick) {
		index = 0;
		if (Trigger) {
			Triggered = true;
			Trigger();
		}
	}
}

void FiniteStateMachine::resetTick() {
	index = 0;
}

unsigned long FiniteStateMachine::getTick() {
	return index;
}

void FiniteStateMachine::setMaxTick(unsigned long value) {
	maxTick = value;
}

unsigned long FiniteStateMachine::getMaxTick() {
	return maxTick;
}

//return the current state
State& FiniteStateMachine::getCurrentState() {
	return *currentState;
}

//return the current state ID
byte FiniteStateMachine::getCurrentStateId() {
	return currentState->getID();
}

//return the previous state
State& FiniteStateMachine::getPreviousState() {
	return *previousState;
}

//return the previous state ID
byte FiniteStateMachine::getPreviousStateID() {
	return previousState->getID();
}

// set the next state in the transition loop
void FiniteStateMachine::transitionNext() {
	 transitionTo(currentState->getNext());
}

//check if state is equal to the currentState
boolean FiniteStateMachine::isInState( State &state ) const {
	if (&state == currentState) {
		return true;
	} else {
		return false;
	}
}

// The trigger function is called when the index reaches max tick, if set
void FiniteStateMachine::setTrigger(void (*TriggerFunction)()){
	index = 0;
	Triggered = false;
	Trigger = TriggerFunction;
}


unsigned long FiniteStateMachine::timeInCurrentState() {
	return millis() - stateChangeTime;
}

bool FiniteStateMachine::wasTriggered(){
	if (Triggered) {
		Triggered = false;
		return true;
	} else {
		return false;
	}
}

//END FINITE STATE MACHINE
