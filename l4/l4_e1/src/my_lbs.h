/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_LBS_H_
#define BT_LBS_H_

/**@file
 * @defgroup bt_lbs LED Button Service API
 * @{
 * @brief API for the LED Button Service (LBS).
 */

#ifdef __cplusplus
extern "C" {
#endif

// Incluye tipos básicos de Zephyr (como bool, uint32_t, etc.)
#include <zephyr/types.h>

/* STEP 1 - Define the 128 bit UUIDs for the GATT service and its characteristics in */
/** @brief LBS Service UUID. */
#define BT_UUID_LBS_VAL \
	BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123) // UUID único para el servicio LBS
/** @brief Button Characteristic UUID. */
#define BT_UUID_LBS_BUTTON_VAL \
	BT_UUID_128_ENCODE(0x00001524, 0x1212, 0xefde, 0x1523, 0x785feabcd123) // UUID único para la característica botón
/** @brief LED Characteristic UUID. */
#define BT_UUID_LBS_LED_VAL \
	BT_UUID_128_ENCODE(0x00001525, 0x1212, 0xefde, 0x1523, 0x785feabcd123) // UUID único para la característica LED
#define BT_UUID_LBS        BT_UUID_DECLARE_128(BT_UUID_LBS_VAL)        // Macro para declarar el UUID del servicio
#define BT_UUID_LBS_BUTTON BT_UUID_DECLARE_128(BT_UUID_LBS_BUTTON_VAL) // Macro para declarar el UUID del botón
#define BT_UUID_LBS_LED    BT_UUID_DECLARE_128(BT_UUID_LBS_LED_VAL)    // Macro para declarar el UUID del LED

/** @brief Callback type for when an LED state change is received. */
typedef void (*led_cb_t)(const bool led_state); // Tipo de función para controlar el LED

/** @brief Callback type for when the button state is pulled. */
typedef bool (*button_cb_t)(void); // Tipo de función para leer el botón

/** @brief Callback struct used by the LBS Service. */
struct my_lbs_cb {
	/** LED state change callback. */
	led_cb_t led_cb;         // Puntero a función para controlar el LED
	/** Button read callback. */
	button_cb_t button_cb;   // Puntero a función para leer el botón
};

/** @brief Initialize the LBS Service.
 *
 * This function registers application callback functions with the My LBS
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
// Función para inicializar el servicio LBS y registrar los callbacks de la app
int my_lbs_init(struct my_lbs_cb *callbacks);

/** @brief Notifica el cambio de estado del botón.
 *
 * Esta función envía una notificación a los clientes conectados
 * cuando el estado del botón cambia.
 *
 * @param[in] button_state El nuevo estado del botón
 */
void my_lbs_send_button_state(bool button_state);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
