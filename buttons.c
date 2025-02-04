/*
 * buttons.c
 *
 *  Created on: Dec 26, 2024
 *      Author: v_
 */
#include "buttons.h"



#define GET_VARIABLE_NAME(Variable) (#Variable)


/*
 * 1. Constantly check if buttons (and which are) pressed - add buttons_check() inside a timer callback
 * 2. When and only pressed - start the timer
 * 3. If counter reached certain value - create action
 * 4. Continue counting
 * 5. When and only when released - stop counting
 */

static uint8_t 	hold_1s_cnt = 0; 						/*!< Count of the actual number of buttons, increased with each successful button register */
//static uint8_t 	hold_2s_count = 0; 						/*!< Count of the actual number of buttons, increased with each successful button register */
//static uint8_t 	hold_5s_count = 0; 						/*!< Count of the actual number of buttons, increased with each successful button register */
//static uint8_t 	hold_10s_count = 0; 					/*!< Count of the actual number of buttons, increased with each successful button register */

uint32_t shared_mask, shared_press_type;


/*
 * @brief 	Detects which pin is pressed
 * 			and adds corresponding mask to the handle.
 * 			State equals to "PRESSED"
 * 			(to 0, RESET state) if
 * 			any of the buttons have been pressed.
 */
static void btns_state_get(Buttons_HandleTypeDef *hbtns) {
	GPIO_PinState gpio_state;
	hbtns->state_current = RELEASED;

	for(int i = 0; i < hbtns->num_of_buttons; ++i) {
		gpio_state = HAL_GPIO_ReadPin(hbtns->list[i].port, hbtns->list[i].pin);

		if(gpio_state == hbtns->list[i].state_active) {
			hbtns->pressed_btns_mask |= hbtns->list[i].mask;
			hbtns->state_current = PRESSED;
		}
	}
}

/*
 * Define current state (usually power button is pressed when this function is called)
 *
 */
static void btns_state_set(Buttons_HandleTypeDef *hbtns, State_TypeDef new_state) {
	hbtns->pressed_btns_mask	= 0; 			// Reset buttons mask
	hbtns->state_current		= new_state;
	hbtns->state_previous		= new_state;
	hbtns->hold_s		= 0;
}

static HAL_StatusTypeDef btns_timer_start(Buttons_HandleTypeDef *hbtns) {
	HAL_TIM_StateTypeDef timer_status = HAL_TIM_Base_GetState(hbtns->htim);

	/*
	 * Start the timer if it hasn't been started yet
	 */
	if (timer_status == HAL_TIM_STATE_READY) {
		/*
		 * Clear pending interrupt flag first
		 * otherwise IT would occur immediately after the start
		 */
		FIX_TIMER_TRIGGER(hbtns->htim);
		timer_status += HAL_TIM_Base_Start_IT(hbtns->htim);

		timer_status += HAL_TIM_OC_Start_IT(hbtns->htim, TIM_CHANNEL_1);
	}

	return (HAL_StatusTypeDef)timer_status;
}

void btns_check(Buttons_HandleTypeDef *hbtns) {
	static uint8_t cnt = 0;
	/*
	 * Get buttons' masks and current state
	 */
	btns_state_get(hbtns);

	/*
	 * Check whether a button state has changed
	 * 1. If changed to active then just notice that (update previous state)
	 * 2. If changed to idle when no long presses were detected - execute "click callback"
	 */
	if (hbtns->state_current != hbtns->state_previous) {
		if (hbtns->state_current == PRESSED) {
			btns_state_set(hbtns, PRESSED);
		} else {
			if (hbtns->hold_s < 2) {
				btns_callback(hbtns->pressed_btns_mask, hbtns->hold_s);
			}
			btns_state_set(hbtns, RELEASED);
		}
	} else {
		/*
		 * If the lvl_en has not changed after a button was pressed
		 * increase counter
		 * and check if it has reached a certain value
		 * and execute "hold callback" when needed.
		 */
		if(hbtns->state_previous == PRESSED) {
			if(++cnt > hold_1s_cnt) {
				cnt = 0;
				++hbtns->hold_s;
				btns_callback(hbtns->pressed_btns_mask, hbtns->hold_s);
			}
		}
	}
}


/*
 * @brief	Set defaults, add timer handle and start it
 */
HAL_StatusTypeDef btns_init(Buttons_HandleTypeDef *hbtns, Button_InitTypeDef user_buttons[], uint8_t num_of_buttons, TIM_HandleTypeDef *htim, State_TypeDef default_state) {
	assert_param(hbtns 			== NULL);
	assert_param(user_buttons 	== NULL);
	assert_param(htim 			== NULL);

	if(num_of_buttons > MAX_NUMBER_OF_BUTTONS)
		return HAL_ERROR;

	hbtns->num_of_buttons	= num_of_buttons;
	hbtns->htim 			= htim;										// Set dedicated to buttons timer

	/*
	 * Register button parameters and fill additional fields
	 */
	for(int id = 0; id < num_of_buttons; ++id) {
		strcpy(hbtns->list[id].name, user_buttons[id].name);		// Copy name
		hbtns->list[id].mask			= 1 << id;					// Create unique bitmask
		hbtns->list[id].port 			= user_buttons[id].port;
		hbtns->list[id].pin 			= user_buttons[id].pin;
		hbtns->list[id].state_active 	= user_buttons[id].state_active;
		hbtns->list[id].state 			= user_buttons[id].state_active == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET; // By default lvl_en is opposite of active
	}

	/*
	 * Set default state to prevent false detection on start.
	 * For example, when buttons are initialized with power button pressed.
	 */
	btns_state_set(hbtns, default_state);

	/*
	 * Set max count for long presses, e.g. 1000ms/50ms = 20
	 */
	hold_1s_cnt 	= 1000/hbtns->htim->Init.Period;
//	hold_2s_count 			= 2000/hbtns->htim->Init.Period;
//	hold_5s_count 			= 5000/hbtns->htim->Init.Period;
//	hold_10s_count 			= 10000/hbtns->htim->Init.Period;

	/*
	 * Start the timer
	 */
	return btns_timer_start(hbtns);
}

/**
 * @brief  Period elapsed callback in non-blocking mode
 * @param  hbtns Buttons handle (struct)
 * @retval None
 */
__weak void btns_callback(uint16_t mask, PressType_TypeDef press_type)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(mask);
	UNUSED(press_type);

	/* NOTE : This function should not be modified, when the callback is needed,
            the buttonsCallback could be implemented in the user file
	 */
}
