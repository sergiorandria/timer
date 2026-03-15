#ifndef __TIMER_APP_H 
#define __TIMER_APP_H 

#include <gtk/gtk.h> 
#include <chrono> 
#include <thread> 
#include <mutex> 
#include <condition_variable> 
#include <atomic> 
#include <iomanip> 
#include <sstream> 
#include <iostream> 
 
#include "timer_class.h" 

class TimerApplication {
private:
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *time_label;
    GtkWidget *nanosec_label;
    GtkWidget *start_button;
    GtkWidget *pause_button;
    GtkWidget *reset_button;
    GtkWidget *spin_button;
    
    // Add main_loop as class member
    GMainLoop *main_loop;
    
    // Timer related
    Time::Timer<float, Time::_Timer_InterObj> timer;
    std::atomic<bool> running{false};
    std::atomic<bool> paused{false};
    std::thread timer_thread;
    std::mutex mtx;
    std::condition_variable cv;
    
    // Time tracking - for countdown
    std::chrono::steady_clock::time_point start_time;
    std::chrono::nanoseconds initial_time{0};     // Total time to count down from
    std::chrono::nanoseconds remaining_time{0};   // Time remaining
    
    // Update interval (16ms ~ 60fps)
    static constexpr auto UPDATE_INTERVAL = std::chrono::milliseconds(16);

public:
    TimerApplication();
    
    ~TimerApplication(); 
    
    void setup_ui(); 
    void run(); 
    
private:
    static void on_window_destroy(GtkWidget *widget, gpointer data); 
    static void on_start_clicked(GtkWidget *widget, gpointer data); 
    static void on_pause_clicked(GtkWidget *widget, gpointer data);  
    static void on_reset_clicked(GtkWidget *widget, gpointer data);
    void start_timer(); 
    void pause_timer(); 
    void reset_timer(); 
    void update_loop();
    void update_display(std::chrono::nanoseconds remaining); 
};
 

#include "timer_app.tpp"
#endif 
