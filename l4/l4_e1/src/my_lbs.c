/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

// Incluye librerías necesarias para tipos, memoria, errores y funciones del sistema
#include <zephyr/types.h>      // Tipos básicos (bool, uint32_t, etc.)
#include <stddef.h>            // Para NULL y size_t
#include <string.h>            // Funciones de manejo de memoria
#include <errno.h>             // Códigos de error estándar
#include <zephyr/sys/printk.h> // Para imprimir mensajes en consola
#include <zephyr/sys/byteorder.h> // Para manejo de orden de bytes
#include <zephyr/kernel.h>     // Funciones del sistema operativo

#include <zephyr/bluetooth/bluetooth.h> // Funciones Bluetooth
#include <zephyr/bluetooth/hci.h>       // Interfaz de controlador Bluetooth
#include <zephyr/bluetooth/conn.h>      // Manejo de conexiones Bluetooth
#include <zephyr/bluetooth/uuid.h>      // Identificadores únicos Bluetooth
#include <zephyr/bluetooth/gatt.h>      // Funciones GATT (servicios y características)

#include "my_lbs.h"                    // Header del servicio LED/Button

#include <zephyr/logging/log.h>         // Para logs

LOG_MODULE_DECLARE(Lesson4_Exercise1);  // Usa el mismo módulo de logs que main.c

static bool button_state;               // Variable para guardar el estado del botón
static bool led_state;                  // Variable para guardar el estado del LED (necesaria para la característica LED)
static struct my_lbs_cb lbs_cb;         // Estructura para guardar los callbacks de la app

/* STEP 6 - Implement the write callback function of the LED characteristic */
// Esta función se llama cuando la app escribe (manda un valor) en la característica LED
static ssize_t write_led(struct bt_conn *conn,
 const struct bt_gatt_attr *attr,
 const void *buf,
 uint16_t len, uint16_t offset, uint8_t flags)
{
 LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, (void *)conn); // Mensaje de depuración
 if (len != 1U) { // Solo se acepta un byte
 LOG_DBG("Write led: Incorrect data length");
 return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN); // Error si la longitud no es 1
 }
 if (offset != 0) { // Solo se acepta offset 0
 LOG_DBG("Write led: Incorrect data offset");
 return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET); // Error si el offset no es 0
 }
 if (lbs_cb.led_cb) { // Si hay callback registrado
 // Lee el valor recibido (0x00 = apagar, 0x01 = encender)
 uint8_t val = *((uint8_t *)buf);
 if (val == 0x00 || val == 0x01) {
 // Llama al callback de la app para actualizar el LED
 lbs_cb.led_cb(val ? true : false);
 } else {
 LOG_DBG("Write led: Incorrect value");
 return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED); // Error si el valor no es válido
 }
 }
 return len; // Devuelve la cantidad de bytes procesados
}
/* STEP 5 - Implement the read callback function of the Button characteristic */
// Esta función se llama cuando la app lee el estado del botón
static ssize_t read_button(struct bt_conn *conn,
 const struct bt_gatt_attr *attr,
 void *buf,
 uint16_t len,
 uint16_t offset)
{
 LOG_DBG("Leyendo estado del botón, handle: %u", attr->handle);
 
 if (lbs_cb.button_cb) {
     // Actualiza el estado del botón llamando al callback
     button_state = lbs_cb.button_cb();
     // Actualiza el valor en user_data
     *(bool *)attr->user_data = button_state;
     
     LOG_DBG("Estado del botón: %s", button_state ? "PRESIONADO" : "SOLTADO");
     
     // Devuelve el valor actualizado
     return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(bool));
 }
 
 LOG_DBG("No hay callback registrado para el botón");
 return 0; // Si no hay callback, devuelve 0
}
/* LED Button Service Declaration */
/* STEP 2 - Create and add the MY LBS service to the Bluetooth LE stack */
// Define el servicio y sus características para Bluetooth
BT_GATT_SERVICE_DEFINE(my_lbs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_LBS), // Servicio principal LBS
	/* STEP 3 - Create and add the Button characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON,    // Característica para leer el botón
		BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, // Permite lectura y notificaciones
		BT_GATT_PERM_READ,                     // Permiso de lectura
		read_button, NULL, &button_state),     // Usa la función read_button y variable button_state
	BT_GATT_CCC(NULL, NULL),                  // Descriptor para habilitar notificaciones
	/* STEP 4 - Create and add the LED characteristic. */
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_LED,    // Característica para controlar el LED
		BT_GATT_CHRC_WRITE,                    // Permite escritura
		BT_GATT_PERM_WRITE,                    // Permiso de escritura
		NULL, write_led, &led_state)           // Usa la función write_led y variable led_state
);
/* A function to register application callbacks for the LED and Button characteristics  */
// Función para registrar los callbacks de la app (LED y botón)
int my_lbs_init(struct my_lbs_cb *callbacks)
{
	if (callbacks) {
		lbs_cb.led_cb = callbacks->led_cb;       // Guarda el callback para el LED
		lbs_cb.button_cb = callbacks->button_cb; // Guarda el callback para el botón
	}
	return 0; // Devuelve éxito
}

// Implementación de la función para notificar el cambio de estado del botón
void my_lbs_send_button_state(bool button_state)
{
	button_state = button_state;

	// Busca la característica del botón en el servicio
	struct bt_gatt_attr *attr = &my_lbs_svc.attrs[2];

	// Envía la notificación a todos los clientes conectados
	bt_gatt_notify(NULL, attr, &button_state, sizeof(button_state));
}
