#include <gtk/gtk.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <iostream>

// Include your existing Timer class
#include "timer_class.h"  // Save your Timer class in this header

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
    std::chrono::steady_clock::time_point paused_time;
    std::chrono::nanoseconds initial_time{0};     // Total time to count down from
    std::chrono::nanoseconds remaining_time{0};   // Time remaining
    
    // Update interval (16ms ~ 60fps)
    static constexpr auto UPDATE_INTERVAL = std::chrono::milliseconds(16);

public:
    TimerApplication() : main_loop(nullptr) {
        setup_ui();
    }
    
    ~TimerApplication() {
        running = false;
        cv.notify_all();
        if (timer_thread.joinable()) {
            timer_thread.join();
        }
    }
    
    void setup_ui() {
        // Create main window
        window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(window), "GTK4 Countdown Timer");
        gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
        g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);
        
        // Create CSS provider for styling
        GtkCssProvider *provider = gtk_css_provider_new();
        const char *css_data = 
            "window { background-color: #2b2b2b; }"
            "#time-label {"
            "   font-size: 48px;"
            "   font-weight: bold;"
            "   font-family: 'Monospace';"
            "   color: #00ff00;"
            "   background-color: #1a1a1a;"
            "   padding: 20px;"
            "   border-radius: 10px;"
            "   margin: 20px;"
            "}"
            "#time-label.warning { color: #ffff00; }"   // Yellow when < 10 seconds
            "#time-label.danger { color: #ff0000; }"    // Red when < 5 seconds
            "#nanosec-label {"
            "   font-size: 24px;"
            "   font-family: 'Monospace';"
            "   color: #888888;"
            "   background-color: #1a1a1a;"
            "   padding: 10px;"
            "   border-radius: 5px;"
            "   margin: 10px;"
            "}"
            "button {"
            "   font-size: 16px;"
            "   padding: 10px 20px;"
            "   margin: 5px;"
            "   border-radius: 5px;"
            "}"
            "#start-button { background-color: #4CAF50; color: white; }"
            "#pause-button { background-color: #ff9800; color: white; }"
            "#reset-button { background-color: #f44336; color: white; }"
            "spinbutton { font-size: 16px; padding: 5px; margin: 10px; }";
        
        gtk_css_provider_load_from_string(provider, css_data);
        
        gtk_style_context_add_provider_for_display(
            gdk_display_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
        g_object_unref(provider);
        
        // Main vertical box
        main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_top(GTK_WIDGET(main_box), 20);
        gtk_widget_set_margin_bottom(GTK_WIDGET(main_box), 20);
        gtk_widget_set_margin_start(GTK_WIDGET(main_box), 20);
        gtk_widget_set_margin_end(GTK_WIDGET(main_box), 20);
        gtk_window_set_child(GTK_WINDOW(window), main_box);
        
        // Time display (minutes:seconds)
        time_label = gtk_label_new("00:00");
        gtk_widget_set_name(time_label, "time-label");
        gtk_box_append(GTK_BOX(main_box), time_label);
        
        // Nanoseconds display
        nanosec_label = gtk_label_new(".000000000");
        gtk_widget_set_name(nanosec_label, "nanosec-label");
        gtk_box_append(GTK_BOX(main_box), nanosec_label);
        
        // Horizontal box for buttons
        GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_box_append(GTK_BOX(main_box), button_box);
        gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
        
        // Start button
        start_button = gtk_button_new_with_label("Start");
        gtk_widget_set_name(start_button, "start-button");
        g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), this);
        gtk_box_append(GTK_BOX(button_box), start_button);
        
        // Pause button
        pause_button = gtk_button_new_with_label("Pause");
        gtk_widget_set_name(pause_button, "pause-button");
        g_signal_connect(pause_button, "clicked", G_CALLBACK(on_pause_clicked), this);
        gtk_box_append(GTK_BOX(button_box), pause_button);
        
        // Reset button
        reset_button = gtk_button_new_with_label("Reset");
        gtk_widget_set_name(reset_button, "reset-button");
        g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_clicked), this);
        gtk_box_append(GTK_BOX(button_box), reset_button);
        
        // Spin button for timer preset (minutes)
        GtkWidget *spin_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_box_append(GTK_BOX(main_box), spin_box);
        gtk_widget_set_halign(spin_box, GTK_ALIGN_CENTER);
        
        GtkWidget *spin_label = gtk_label_new("Set minutes:");
        gtk_box_append(GTK_BOX(spin_box), spin_label);
        
        GtkAdjustment *adjustment = gtk_adjustment_new(1, 0, 60, 1, 5, 0); // Default 1 minute
        spin_button = gtk_spin_button_new(adjustment, 1, 0);
        gtk_box_append(GTK_BOX(spin_box), spin_button);
        
        // Initialize display
        update_display(std::chrono::nanoseconds(0));
    }
    
    void run() {
        gtk_widget_set_visible(window, TRUE);
        
        // Start update thread
        running = true;
        timer_thread = std::thread(&TimerApplication::update_loop, this);
        
        // Create and run main loop
        main_loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(main_loop);
        g_main_loop_unref(main_loop);
        main_loop = nullptr;
    }
    
private:
    static void on_window_destroy(GtkWidget *widget, gpointer data) {
        TimerApplication *app = static_cast<TimerApplication*>(data);
        app->running = false;
        if (app->main_loop && g_main_loop_is_running(app->main_loop)) {
            g_main_loop_quit(app->main_loop);
        }
    }
    
    static void on_start_clicked(GtkWidget *widget, gpointer data) {
        TimerApplication *app = static_cast<TimerApplication*>(data);
        app->start_timer();
    }
    
    static void on_pause_clicked(GtkWidget *widget, gpointer data) {
        TimerApplication *app = static_cast<TimerApplication*>(data);
        app->pause_timer();
    }
    
    static void on_reset_clicked(GtkWidget *widget, gpointer data) {
        TimerApplication *app = static_cast<TimerApplication*>(data);
        app->reset_timer();
    }
    
    void start_timer() {
        if (!running) return;
        
        std::lock_guard<std::mutex> lock(mtx);
        if (paused) {
            // Resume from pause
            paused = false;
            start_time = std::chrono::steady_clock::now();
        } else {
            // Start from beginning with preset
            int minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_button));
            initial_time = std::chrono::minutes(minutes);
            remaining_time = initial_time;  // Set remaining time to initial time
            start_time = std::chrono::steady_clock::now();
            
            // Debug output
            std::cout << "Timer started with " << minutes << " minutes ("
                      << initial_time.count() << " nanoseconds)" << std::endl;
        }
        cv.notify_all();
        
        // Update button states
        gtk_widget_set_sensitive(start_button, FALSE);
        gtk_widget_set_sensitive(pause_button, TRUE);
        gtk_widget_set_sensitive(spin_button, FALSE);
    }
    
    void pause_timer() {
        std::lock_guard<std::mutex> lock(mtx);
        if (!paused) {
            paused = true;
            // Store the remaining time when paused
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - start_time;
            remaining_time = initial_time - std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
            if (remaining_time < std::chrono::nanoseconds(0)) {
                remaining_time = std::chrono::nanoseconds(0);
            }
        }
        
        // Update button states
        gtk_widget_set_sensitive(start_button, TRUE);
        gtk_widget_set_sensitive(pause_button, FALSE);
        gtk_widget_set_sensitive(spin_button, TRUE);  // Re-enable spin button
    }
    
    void reset_timer() {
        std::lock_guard<std::mutex> lock(mtx);
        initial_time = std::chrono::nanoseconds(0);
        remaining_time = std::chrono::nanoseconds(0);
        paused = false;
        
        // Update display immediately
        update_display(std::chrono::nanoseconds(0));
        
        // Update button states
        gtk_widget_set_sensitive(start_button, TRUE);
        gtk_widget_set_sensitive(pause_button, FALSE);
        gtk_widget_set_sensitive(spin_button, TRUE);
    }
    
    void update_loop() {
        while (running) {
            bool should_update = false;
            std::chrono::nanoseconds current_remaining{0};
            
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (!paused && running && remaining_time.count() > 0) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = now - start_time;
                    current_remaining = initial_time - std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
                    
                    if (current_remaining < std::chrono::nanoseconds(0)) {
                        current_remaining = std::chrono::nanoseconds(0);
                        remaining_time = std::chrono::nanoseconds(0);
                        paused = true;  // Auto-pause at zero
                        
                        // Schedule button state update
                        g_idle_add([](gpointer data) -> gboolean {
                            auto *app = static_cast<TimerApplication*>(data);
                            gtk_widget_set_sensitive(app->start_button, TRUE);
                            gtk_widget_set_sensitive(app->pause_button, FALSE);
                            gtk_widget_set_sensitive(app->spin_button, TRUE);
                            return G_SOURCE_REMOVE;
                        }, this);
                    } else {
                        remaining_time = current_remaining;
                    }
                    should_update = true;
                }
            }
            
            if (should_update) {
                // Update display in GTK main thread
                g_idle_add([](gpointer data) -> gboolean {
                    auto *app = static_cast<TimerApplication*>(data);
                    std::lock_guard<std::mutex> lock(app->mtx);
                    app->update_display(app->remaining_time);
                    return G_SOURCE_REMOVE;
                }, this);
            }
            
            // Sleep for update interval
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = UPDATE_INTERVAL.count() * 1000000;
            timer.__nanosleep(&ts, nullptr);
        }
    }
        
    void update_display(std::chrono::nanoseconds remaining) {
        // Extract minutes, seconds, and nanoseconds
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(remaining);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(remaining - minutes);
        auto nanoseconds = remaining - minutes - seconds;
        
        // Format main time display (MM:SS)
        std::stringstream time_ss;
        time_ss << std::setw(2) << std::setfill('0') << minutes.count() << ":"
                << std::setw(2) << std::setfill('0') << seconds.count();
        
        // Format nanoseconds display (.nnnnnnnnn)
        std::stringstream nano_ss;
        nano_ss << "." << std::setw(9) << std::setfill('0') 
                << nanoseconds.count();
        
        // Update labels
        gtk_label_set_text(GTK_LABEL(time_label), time_ss.str().c_str());
        gtk_label_set_text(GTK_LABEL(nanosec_label), nano_ss.str().c_str());
        
        // Change color based on remaining time
        if (remaining.count() <= 0) {
            gtk_widget_remove_css_class(time_label, "warning");
            gtk_widget_remove_css_class(time_label, "danger");
        } else if (remaining < std::chrono::seconds(5)) {
            gtk_widget_add_css_class(time_label, "danger");
            gtk_widget_remove_css_class(time_label, "warning");
        } else if (remaining < std::chrono::seconds(10)) {
            gtk_widget_add_css_class(time_label, "warning");
            gtk_widget_remove_css_class(time_label, "danger");
        } else {
            gtk_widget_remove_css_class(time_label, "warning");
            gtk_widget_remove_css_class(time_label, "danger");
        }
    }
};

int main(int argc, char *argv[]) {
    // Initialize GTK - GTK4 uses gtk_init() instead of gtk_init(&argc, &argv)
    gtk_init();
    
    // Create and run application
    TimerApplication app;
    app.run();
    
    return 0;
}
