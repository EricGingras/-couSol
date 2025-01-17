/*
Ce code est destiné à créer un point d'accès Wi-Fi avec un serveur web embarqué sur un microcontrôleur ESP32. Voici une description des différentes parties du code :
Les gestionnaires d'événements recordOFF_handler et recordON_handler sont des fonctions qui sont exécutées lorsque certaines pages Web sont ouvertes. 
Ces fonctions contrôlent les broches GPIO pour allumer ou éteindre une LED.

La section "SECTION HTML" contient des définitions de chaînes de caractères représentant le code HTML des différentes pages du serveur web. 
Ces pages affichent des boutons pour allumer ou éteindre la LED.

La fonction start_webserver configure et lance le serveur web en utilisant les gestionnaires d'événements définis précédemment.

Les fonctions wifi_event_handler, wifi_init_softap et start_wifi_softap sont utilisées pour configurer et démarrer le point d'accès Wi-Fi. 
Elles définissent le SSID, le mot de passe, le canal et d'autres paramètres du point d'accès.

La fonction app_main est la fonction principale du programme. Elle initialise le point d'accès Wi-Fi et le serveur web, 
puis met en attente les événements du système.

En résumé, ce code crée un point d'accès Wi-Fi avec un serveur web embarqué qui contrôle une LED à l'aide de boutons affichés sur les pages web. 
L'utilisateur peut allumer ou éteindre la LED en accédant aux pages web via le point d'accès Wi-Fi créé par l'ESP32.

Matériel requis: esp32
Écrit par Jacob Turcotte
*/
//---------------------------DÉBUT LIBRAIRIES------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_netif.h"
#include <esp_http_server.h>
#include "esp_err.h"

#include <sys/param.h>

#include "nvs_flash.h"

#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>
//---------------------------FIN LIBRAIRIES------------------------------//


// Definition des configuration pour le PA voir fichier Kconfig.projbuild
#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID //ssid du wifi
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD //mot de passe du wifi
#define ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL //channel du wifi utilisé
#define MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN //nombre maximal de connection 

#define LED GPIO_NUM_2

static const char *TAG = "wifi softAP"; //Définition d'un tag pour les erreurs

//-------------------------------HANDLERS----------------------------------//
/// @brief Évènement qui s'exécute lorsque la page est ouverte. On peut envoyer un message dans le terminal ou bien accèder à
/// des pins GPIO. Celui-ci est pour quand l'enregistrement est à OFF. Il arrête un enregistrement si il en a un en cours.
/// @param req requête html
/// @return error, retourne un code d'erreur
static esp_err_t recordOFF_handler(httpd_req_t *req)
{
	esp_err_t error;
    //CODE ÉRIC DÉBUT

	ESP_LOGI(TAG, "LED Turned OFF");
	gpio_set_level(LED, 0);


    //CODE ÉRIC FIN
	const char *response = (const char *) req->user_ctx;
	error = httpd_resp_send(req, response, strlen(response));
	if (error != ESP_OK)
	{
		ESP_LOGI(TAG, "Error %d while sending Response", error);
	}
	else ESP_LOGI(TAG, "Response sent Successfully");
	return error;
}

/// @brief Évènement qui s'exécute lorsque la page est ouverte. On peut envoyer un message dans le terminal ou bien accèder à
/// des pins GPIO. Celui-ci commence un nouvel enregistrement.
/// @param req requête html
/// @return error, retourne un code d'erreur
static esp_err_t recordON_handler(httpd_req_t *req)
{
	esp_err_t error;
    //CODE ÉRIC DÉBUT

	ESP_LOGI(TAG, "LED Turned ON");
	gpio_set_level(LED, 1);


    //CODE ÉRIC FIN
	const char *response = (const char *) req->user_ctx;
	error = httpd_resp_send(req, response, strlen(response));
	if (error != ESP_OK)
	{
		ESP_LOGI(TAG, "Error %d while sending Response", error);
	}
	else ESP_LOGI(TAG, "Response sent Successfully");
	return error;
}
//------------------------------FIN HANDLERS-------------------------------//

//---------------------------DÉBUT SECTION HTML----------------------------//
static const httpd_uri_t recordOff = {
    .uri       = "/recordoff",
    .method    = HTTP_GET,
    .handler   = recordOFF_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #FF0000;} /* Red */\
</style>\
</head>\
<body>\
\
<h1>ESP32 WEBSERVER</h1>\
<p>Arret de l'enregistrement.</p>\
<h3>Appuyez sur le bouton afin de commencer un nouvel enregistrement.</h3>\
\
<button class=\"button button1\" onclick= \"window.location.href='/recordon'\">RECORD</button>\
\
</body>\
</html>"
};

static const httpd_uri_t recordOn = {
    .uri       = "/recordon",
    .method    = HTTP_GET,
    .handler   = recordON_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #000000;} /* Black */\
</style>\
</head>\
<body>\
\
<h1>ESP32 WEBSERVER</h1>\
<p>Enregistrement en cours ...</p>\
<h3>Temps d'enregistrement : (a faire eventuellement).</h3>\
\
<button class=\"button button1\" onclick= \"window.location.href='/recordoff'\">STOP</button>\
\
</body>\
</html>"
};

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = recordOFF_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #FF0000;} /* Red */\
</style>\
</head>\
<body>\
\
<h1>ESP32 WEBSERVER</h1>\
<p>Arret de l'enregistrement.</p>\
<h3>Appuyez sur le bouton afin de commencer un nouvel enregistrement.</h3>\
\
<button class=\"button button1\" onclick= \"window.location.href='/recordon'\">RECORD</button>\
\
</body>\
</html>"
};
//----------------------------FIN SECTION HTML-----------------------------//

//---------------------------GESTION SERVER WEB----------------------------//
/// @brief Cette fonction gère les erreurs 404 dans un serveur HTTP en envoyant une réponse d'erreur 404 avec un message personnalisé.
/// @param req Un pointeur vers une structure httpd_req_t représentant la requête HTTP.
/// @param err Le code d'erreur httpd_err_code_t correspondant à l'erreur rencontrée.
/// @return Un code d'erreur esp_err_t indiquant le statut de l'opération (ESP_FAIL en cas d'erreur).
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}
/// @brief Cette fonction démarre un serveur HTTP en utilisant la configuration par défaut et enregistre des gestionnaires d'URI spécifiques.
/// @return Un pointeur vers une structure httpd_handle_t représentant le serveur HTTP démarré avec succès, ou NULL en cas d'erreur.
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &recordOff);
        httpd_register_uri_handler(server, &recordOn);
        httpd_register_uri_handler(server, &root);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

/// @brief Cette fonction arrête un serveur HTTP spécifié.
/// @param server Un pointeur vers une structure httpd_handle_t représentant le serveur HTTP à arrêter.
static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

/// @brief Événement qui s'exécute lorsqu'une déconnexion réseau est détectée. Il arrête le serveur HTTP s'il est en cours d'exécution.
/// @param arg Un pointeur vers une structure httpd_handle_t* représentant le serveur HTTP à arrêter.
/// @param event_base La base de l'événement qui a déclenché cet événement.
/// @param event_id L'ID de l'événement qui a été déclenché.
/// @param event_data Les données de l'événement associé.
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

/// @brief Événement qui s'exécute lorsqu'une connexion réseau est détectée. Il démarre le serveur HTTP s'il n'est pas déjà en cours d'exécution.
/// @param arg Un pointeur vers une structure httpd_handle_t* représentant le serveur HTTP à démarrer.
/// @param event_base La base de l'événement qui a déclenché cet événement.
/// @param event_id L'ID de l'événement qui a été déclenché.
/// @param event_data Les données de l'événement associé.
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}
//------------------------FIN GESTION SERVER WEB---------------------------//

//À enlever plus tard
static void configure_led (void)
{
	gpio_reset_pin (LED);

	gpio_set_direction (LED, GPIO_MODE_OUTPUT);
}

//------------------------------DÉBUT PA-----------------------------------//

/// @brief Cette fonction est un gestionnaire d'événements WiFi qui est appelé lorsqu'un appareil se connecte ou se déconnecte du point d'accès (PA).
/// @param arg Un pointeur vers des données supplémentaires.
/// @param event_base La base de l'événement qui a déclenché cet événement.
/// @param event_id L'ID de l'événement qui a été déclenché.
/// @param event_data Les données de l'événement associé.
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    //Lorsqu'un appareil se connecte au PA
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        //affiche dans le terminal
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } 
    //Lorsqu'un appareil se déconnecte du PA
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        //affiche dans le terminal
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

/// @brief Cette fonction initialise le point d'accès (PA) WiFi.
/// @details Elle initialise le réseau, crée une boucle d'événements par défaut, configure le PA WiFi avec les paramètres spécifiés, enregistre le gestionnaire d'événements WiFi et démarre le PA.
/// @return Rien.
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    //Crée un loop d'évènement. Or, quand un évènement est généré il execute la fonction connectée à l'évènement.
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t * p_netif = esp_netif_create_default_wifi_ap();

    //crée "l'objet"(utilise une structur) wifi cfg
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //initialise "l'objet" cfg pour faire le point d'accès
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //Rajout d'un évènement à la loop. Voir fonction wifi_event_handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    //Configure la structure pour le wifi
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID, //"nom" du Wifi
            .ssid_len = strlen(ESP_WIFI_SSID), //Longueur du nom
            .channel = ESP_WIFI_CHANNEL, //Selection du channel Wifi
            .password = ESP_WIFI_PASS, //Définition du mot de passe
            .max_connection = MAX_STA_CONN, //Définiton du nombre de connection maximal
            .authmode = WIFI_AUTH_WPA_WPA2_PSK, //définiton du type d'authentification
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    //Si il a pas de mot de passe, on change l'anthentification pour ne pas en demander un
    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    //met le wifi en PA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    //"active" la configuration définie dans la structure wifi_config 
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    //Commence le PA
    ESP_ERROR_CHECK(esp_wifi_start());
    //Message dans le terminal
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
    //Montre l'adresse IP dans le terminal
    esp_netif_ip_info_t if_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(p_netif, &if_info));
    ESP_LOGI(TAG, "ESP32 IP:" IPSTR, IP2STR(&if_info.ip));
}

/// @brief Cette fonction démarre le point d'accès (PA) WiFi.
/// @details Elle initialise NVS (Non-volatile Storage), puis initialise et démarre le PA WiFi.
/// @return Rien.
void start_wifi_softap(void)
{
    //Initialize NVS (Non-volatile Storage) dois faire plus de recherche pour son utilité
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    //Affiche terminal
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    //Initialise le AP
    wifi_init_softap();
}
//--------------------------------FIN PA-----------------------------------//

void app_main(void)
{
    static httpd_handle_t server = NULL;
	configure_led(); //à enlever plus tard

    start_wifi_softap();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));
//    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

}
