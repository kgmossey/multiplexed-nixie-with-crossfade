/*
||
|| @file FiniteStateMachine.h
|| @version 1.7
|| @author Alexander Brevig
|| @contact alexanderbrevig@gmail.com
||
|| @description
|| | Provide an easy way of making finite state machines
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
||
*/

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <Arduino.h>

#define NO_ENTER (0)
#define NO_UPDATE (0)
#define NO_EXIT (0)

#define FSM FiniteStateMachine

//define the functionality of the states
class State {
	public:
		State( void (*updateFunction)() );
		State( void (*enterFunction)(), void (*updateFunction)(), void (*exitFunction)() );
		State( byte stateID );
		State( byte stateID, void (*updateFunction)() );
		State( byte stateID, void (*enterFunction)(), void (*updateFunction)(), void (*exitFunction)() );

		byte getID();
		void enter();
		void update();
		void exit();
		State& getNext();
		void setNext(State& next);

	private:
		byte id;
		void (*userEnter)();
		void (*userUpdate)();
		void (*userExit)();
		State *nextState;
};

//define the finite state machine functionality
class FiniteStateMachine {
	public:
		FiniteStateMachine(State& current);

		FiniteStateMachine& update();
		FiniteStateMachine& transitionTo( State& state );
		// made transitionTo work like the following
		//FiniteStateMachine& immediateTransitionTo( State& state );
		void tick();
		void setMaxTick(unsigned long value);
		byte getTick();
		void resetTick();

		State& getCurrentState();
		byte getCurrentStateId();
		State& getPreviousState();
		byte getPreviousStateID();
		void transitionNext();
		boolean isInState( State &state ) const;
		void setTrigger(void (*TriggerFunction)());

		unsigned long timeInCurrentState();

	private:
		bool 	needToTriggerEnter;
		State* 	previousState;
		State* 	currentState;
		State* 	nextState;
		void (*Trigger)();
		unsigned long stateChangeTime;
		unsigned long index = 0;
		unsigned long maxTick = 10;
};

#endif
