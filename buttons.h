/*
 * buttons.h
 *
 *  Created on: Dec 26, 2024
 *      Author: v_
 */

#ifndef INC_BUTTONS_H_
#define INC_BUTTONS_H_

#include <string.h>
#include "main.h"

#define FIX_TIMER_TRIGGER(handle_ptr) 	(__HAL_TIM_CLEAR_FLAG(handle_ptr, TIM_SR_UIF))

#define MAX_NUMBER_OF_BUTTONS	15	// 15 is max because "mask" is a 16-bit variable
#define MAX_NAME_LENGTH			20

extern uint32_t shared_mask, shared_press_type;

typedef enum {
	RELEASED,
	PRESSED
} State_TypeDef;

typedef enum {
	CLICK,
	HOLD,
	NUM_OF_PRESS_TYPES
} PressType_TypeDef;

typedef struct {
	char				name[MAX_NAME_LENGTH];				/*!< Button name (its purpose), for e.g. "Down    "				*/
	GPIO_TypeDef*		port;
	uint16_t 			pin;
	GPIO_PinState 		state_active;			/*!< State of a button when is it activated by user				*/
} Button_InitTypeDef;

typedef struct {
	struct {													/*!< List of registered buttons									*/
		char				name[MAX_NAME_LENGTH];				/*!< Button name (its purpose), for e.g. "Down    "				*/
		uint16_t			mask;								/*!< List of currently pressed buttons presented as bitmask		*/
		GPIO_TypeDef*		port;
		uint16_t 			pin;
		GPIO_PinState 		state_active;						/*!< State of a button when is it activated by user				*/
		GPIO_PinState 		state;								/*!< State of a button when is it activated by user				*/
	} list[MAX_NUMBER_OF_BUTTONS];
	uint8_t 					num_of_buttons;					/*!< Registered number of buttons								*/
	uint16_t					pressed_btns_mask;				/*!< List of currently pressed buttons presented as bitmask		*/
	State_TypeDef 				state_current;					/*!< Pressed or Released current state of all buttons			*/
	State_TypeDef 				state_previous;					/*!< Pressed or Released previous state of all buttons			*/
	uint8_t 					hold_s;							/*!< Current amount of seconds a button has been pressed for	*/
	TIM_HandleTypeDef			*htim;							/*!< Button timer (e.g. &htim1)									*/
} Buttons_HandleTypeDef;

HAL_StatusTypeDef 	btns_init		(Buttons_HandleTypeDef *hbtns, Button_InitTypeDef user_buttons[], uint8_t num_of_buttons, TIM_HandleTypeDef *htim, State_TypeDef default_state);
void	 			btns_check		(Buttons_HandleTypeDef *hbuttons);
void 				btns_callback	(uint16_t mask, uint8_t press_duration_s);

#endif /* INC_BUTTONS_H_ */
