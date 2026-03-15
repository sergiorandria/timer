#include "timer_app.h"

int main(int argc, char *argv[]) {
    gtk_init();
    
    TimerApplication app;
    app.run();
    
    return 0;
}
