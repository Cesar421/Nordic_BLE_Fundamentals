/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// Incluye las librerías necesarias para el sistema operativo, Bluetooth y control de LEDs/Botones
#include <zephyr/kernel.h>              // Funciones básicas del sistema operativo
#include <zephyr/logging/log.h>         // Para mostrar mensajes en consola
#include <zephyr/bluetooth/bluetooth.h> // Funciones Bluetooth
#include <zephyr/bluetooth/gap.h>       // Parámetros de conexión Bluetooth
#include <zephyr/bluetooth/uuid.h>      // Identificadores únicos Bluetooth
#include <zephyr/bluetooth/conn.h>      // Manejo de conexiones Bluetooth
#include <dk_buttons_and_leds.h>        // Control de LEDs y botones en la placa
/* STEP 7 - Include the header file of MY LBS customer service */
#include "my_lbs.h"                    // Servicio personalizado para controlar LED y leer botón

// Referencia al servicio LBS definido en my_lbs.c
extern const struct bt_gatt_service_static my_lbs_svc;

// Configura los parámetros de publicidad Bluetooth (anuncios para que otros dispositivos te encuentren)
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY), // Permite conexiones y usa la dirección del dispositivo
	800, // Intervalo mínimo de publicidad (800 * 0.625ms = 500ms)
	801, // Intervalo máximo de publicidad (801 * 0.625ms = 500.625ms)
	NULL // Publicidad no dirigida (cualquier dispositivo puede conectarse)
);

// Inicializa el módulo de logs para mostrar mensajes en consola
LOG_MODULE_REGISTER(Lesson4_Exercise1, LOG_LEVEL_INF);

// Nombre del dispositivo Bluetooth (se muestra cuando otros buscan dispositivos)
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// LEDs de la placa para mostrar estados
#define RUN_STATUS_LED DK_LED1      // LED que parpadea para indicar que el programa está corriendo
#define CON_STATUS_LED DK_LED2      // LED que indica si hay conexión Bluetooth

/* STEP 8.1 - Specify the LED to control */
#define USER_LED DK_LED3            // LED controlado por la app (por Bluetooth)
/* STEP 9.1 - Specify the button to monitor */
#define USER_BUTTON DK_BTN1_MSK     // Botón que se lee desde la app
#define RUN_LED_BLINK_INTERVAL 1000 // Intervalo de parpadeo del LED de estado (en milisegundos)

static bool app_button_state;       // Variable para guardar el estado del botón
static struct k_work adv_work;      // Estructura para manejar el trabajo de publicidad Bluetooth

// Datos que se anuncian por Bluetooth (nombre y flags)
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)), // Indica que es un dispositivo general y solo usa Bluetooth Low Energy
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),         // Anuncia el nombre del dispositivo
};

// Datos adicionales que se anuncian (UUID del servicio personalizado)
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL), // Anuncia el servicio LED Button Service
};

// Función que inicia la publicidad Bluetooth (permite que otros dispositivos te encuentren)
static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd)); // Inicia la publicidad con los parámetros y datos definidos

	if (err) {
		printk("Advertising failed to start (err %d)\n", err); // Si falla, muestra error
		return;
	}

	printk("Advertising successfully started\n"); // Si funciona, muestra mensaje de éxito
}
// Función para programar el inicio de la publicidad Bluetooth
static void advertising_start(void)
{
	k_work_submit(&adv_work); // Encola el trabajo para que se ejecute
}
// Callback que se llama cuando una conexión previa se recicla (desconexión completa)
static void recycled_cb(void)
{
	printk("Connection object available from previous conn. Disconnect is complete!\n"); // Mensaje informativo
	advertising_start(); // Vuelve a iniciar la publicidad para aceptar nuevas conexiones
}

/* STEP 8.2 - Define the application callback function for controlling the LED */
static void app_led_cb(bool led_state)
{
	dk_set_led(USER_LED, led_state); // Enciende o apaga el LED según el estado recibido
}
/* STEP 9.2 - Define the application callback function for reading the state of the button */
static bool app_button_cb(void)
{
	return app_button_state; // Devuelve el estado actual del botón
}
/* STEP 10 - Declare a varaible app_callbacks of type my_lbs_cb and initiate its members to the applications call back functions we developed in steps 8.2 and 9.2. */
static struct my_lbs_cb app_callbacks = {
	.led_cb    = app_led_cb,    // Controla el LED
	.button_cb = app_button_cb, // Lee el botón
};
// Función que se llama cuando cambia el estado de los botones
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) { // Si el botón de usuario cambió
		uint32_t user_button_state = button_state & USER_BUTTON; // Extrae el estado del botón
		app_button_state = user_button_state ? true : false;     // Actualiza el estado del botón
		printk("Estado del botón cambiado: %s\n", app_button_state ? "PRESIONADO" : "SOLTADO");

		// Notifica el cambio de estado a través de Bluetooth
		my_lbs_send_button_state(app_button_state);
	}
}
// Callback que se llama cuando se establece una conexión Bluetooth
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err); // Si falla, muestra error
		return;
	}

	printk("Connected\n"); // Si funciona, muestra mensaje de éxito
	dk_set_led_on(CON_STATUS_LED); // Enciende el LED de estado de conexión
}

// Callback que se llama cuando se pierde la conexión Bluetooth
static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason); // Muestra motivo de desconexión
	dk_set_led_off(CON_STATUS_LED); // Apaga el LED de estado de conexión
}

// Estructura que agrupa los callbacks de conexión Bluetooth
struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,     // Se llama al conectar
	.disconnected = on_disconnected, // Se llama al desconectar
	.recycled = recycled_cb,       // Se llama al reciclar conexión
};

// Inicializa los botones y registra la función que se llama cuando cambian
static int init_button(void)
{
	int err;
	err = dk_buttons_init(button_changed); // Registra la función para manejar cambios de botón
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err); // Muestra error si falla
	}
	return err;
}

// Función principal del programa (punto de entrada)
int main(void)
{
	int blink_status = 0; // Variable para alternar el parpadeo del LED
	int err;

	LOG_INF("Starting Lesson 4 - Exercise 1 \n"); // Mensaje de inicio

	// Inicializa los LEDs de la placa
	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}

	// Inicializa los botones y registra el callback
	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return -1;
	}

	// Inicializa el Bluetooth
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}
	bt_conn_cb_register(&connection_callbacks); // Registra los callbacks de conexión

	/* STEP 11 - Pass your application callback functions stored in app_callbacks to the MY LBS service */
	err = my_lbs_init(&app_callbacks); // Inicializa el servicio personalizado y le pasa los callbacks
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return -1;
	}
	LOG_INF("Bluetooth initialized\n"); // Mensaje de éxito

	// Inicializa el trabajo para publicidad Bluetooth
	k_work_init(&adv_work, adv_work_handler);
	advertising_start(); // Comienza a anunciar el dispositivo

	// Bucle principal: parpadea el LED de estado para mostrar que el programa está corriendo
	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2); // Cambia el estado del LED
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));          // Espera un segundo
	}
}
