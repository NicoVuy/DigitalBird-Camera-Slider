#ifndef COAP_SERVER_H
#define COAP_SERVER_H

#include <WiFiUdp.h>    // For UDP
#include <coap-simple.h>  // CoAP simple library from https://github.com/hirotakaster/CoAP-simple-library


void coap_callback_discovery(CoapPacket &packet, IPAddress ip, int port);

class CoapServer {
public:
    CoapServer(Logger &alogger):is_started(false),coap(coapudp),logger(alogger){
        instance=this;
    }

    void begin() {
        logger.println("Starting CoAP server");
        IPAddress multicastIP(224, 0, 1, 187);
        coapudp.beginMulticast(multicastIP,5683); // Join multicast group

        // Register server callbacks
        logger.println("Setting up CoAP callbacks");
        coap.server(coap_callback_discovery, ".well-known/core");

        // Start CoAP
        // coap.start(); dont do this, it will overwrite the beginMulticast
        is_started=true;
    }

    void end() {
        coap=Coap(coapudp);

        is_started=false;
    }

    void loop() {
        // Process incoming CoAP packets
        if (is_started) {
            coap.loop();
        }
    }

    void callback_discovery(CoapPacket &packet, IPAddress ip, int port) {
        logger.println("Received CoAP request, responding...");
        if (packet.code == COAP_GET) {
        logger.println("Received CoAP discovery request, responding...");
        const char* payload = "</gimbal>;title=\"DigitalBird DB3 Wifi\";rt=\"gimbal\";if=\"VISCA\";ct=0";
        coap.sendResponse(ip, port, packet.messageid, payload, strlen(payload),
                            COAP_CONTENT, COAP_APPLICATION_LINK_FORMAT, packet.token, packet.tokenlen);
        } else {
        logger.println("Received unspported CoAP request, responding...");
        coap.sendResponse(ip, port, packet.messageid, NULL, 0,
                            COAP_METHOD_NOT_ALLOWD, COAP_NONE, packet.token, packet.tokenlen);
        }
    }
private:
    bool is_started;
    WiFiUDP coapudp;
    Logger &logger;
    Coap coap;
public:
    static CoapServer* instance;
    
};

CoapServer* CoapServer::instance = nullptr;

void coap_callback_discovery(CoapPacket &packet, IPAddress ip, int port) {
    CoapServer::instance->callback_discovery(packet, ip, port);
}



#endif // COAP_SERVER_H