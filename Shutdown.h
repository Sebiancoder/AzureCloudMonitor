const int SHUTDOWN_BUTTON_PIN = 27;

extern bool shutdownVMs;

void IRAM_ATTR handleShutdownPress() {

  shutdownVMs = true;

}