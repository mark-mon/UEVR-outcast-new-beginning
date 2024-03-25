#define  _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <ctime>
#include <iostream>

#include "uevr/Plugin.hpp"
#include "DialogueManager.hpp"
#include "CinematicManager.hpp"

#define DELAY_VALUE 2000

typedef struct _TIMER_STRUCT
{
    bool* XPressed;
    std::time_t* TimeStamp;
}   TIMER_STRUCT;

typedef enum _TP_CAUSE {
    CAUSE_NONE = 0,
    CAUSE_X,
    CAUSE_DIALOGUE,
    CAUSE_CINEMATIC
} TP_CAUSE;

void DebugPrint(char* Format, ...);
using namespace uevr;

#define PLUGIN_LOG_ONCE(...) \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_info(__VA_ARGS__); \
    }


DWORD WINAPI
TimerCallbackThreadProc(
  LPVOID    lpParameter   // thread data
    )
{
    TIMER_STRUCT*   ThisTimer = (TIMER_STRUCT*)lpParameter;
    static bool ThirdPerson = false;
    
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_F4;
    
    while(1) {
        Sleep(10);
        
        if (*ThisTimer->XPressed == true) {
            ThirdPerson = true;
            input.ki.dwFlags = 0;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(50);
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
        
        while(ThirdPerson == true) {
            Sleep(DELAY_VALUE);
            
            if(ThirdPerson == false) break;
            
            // See if the timestamp is still the same or has been updated.
            if(std::time(0) >= *ThisTimer->TimeStamp + 2) {
                *ThisTimer->XPressed = false;
                ThirdPerson = false;
                input.ki.dwFlags = 0;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(50);
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }

    /* Needs to return a value, and we're not using this */
    return 0;
}


class UevrPlugin : public uevr::Plugin {
public:
    std::time_t m_TimeStamp;
    TIMER_STRUCT m_Timer;
    bool m_XPressed;
    UevrPlugin() = default;

    void on_dllmain() override {
        ZeroMemory(&m_Timer, sizeof(TIMER_STRUCT));
        m_Timer.XPressed = &m_XPressed;
        m_Timer.TimeStamp = &m_TimeStamp;
    }

    void on_initialize() override {
           
        DWORD ThreadId  = 0;
        
        // Start the monitoring thread
        CreateThread(NULL,
                     0,
                     (LPTHREAD_START_ROUTINE)(&TimerCallbackThreadProc),
                     &m_Timer,
                     0,
                     &ThreadId);

    }

    //*******************************************************************************************
    // This is the controller input routine. Everything happens here.
    //*******************************************************************************************
    void on_xinput_get_state(uint32_t* retval, uint32_t user_index, XINPUT_STATE* state) {
        
        static TP_CAUSE Cause = CAUSE_NONE;
 		static bool FirstPerson = true;
        static bool KeyDown = false;
        bool IsInDialogue = false;
        bool IsInCinematic = false;
        static bool XDown = false;
        
        INPUT input;
        ZeroMemory(&input, sizeof(INPUT));
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_F4;

        if(KeyDown == true){
            KeyDown = false;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
        
        if(state != NULL) {
            
            // Check if we are in 3rd person and if so, if we should switch back to first person.    
            if(FirstPerson == false && Cause >= CAUSE_DIALOGUE) {
                // Check if 3rd person due to dialog
                if(Cause == CAUSE_DIALOGUE) {
                    const auto DM = DialogueManager::get_instance();
                    if(DM) {
                        IsInDialogue = DM->is_in_dialogue();
                        if(!IsInDialogue) {
                           KeyDown = true;
                           input.ki.dwFlags = 0;
                           SendInput(1, &input, sizeof(INPUT));
                           FirstPerson = true;
                           Cause = CAUSE_NONE;
                           API::get()->log_info("In Dialog=%d, In 3rd, switching to 1st person", IsInCinematic);
                        }
                    }
                // Check if 3rd person due to cinematic sequence.
                } else if(Cause == CAUSE_CINEMATIC) {
                    const auto CM = CinematicManager::get_instance();
                    if(CM) {
                        IsInCinematic = CM->is_in_cinematic();
                        if(!IsInCinematic) {
                           KeyDown = true;
                           input.ki.dwFlags = 0;
                           SendInput(1, &input, sizeof(INPUT));
                           FirstPerson = true;
                           Cause = CAUSE_NONE;
                           API::get()->log_info("In Cinematic=%d, In 3rd, switching to 1st person", IsInCinematic);
                        }
                    } 
                }
            } 


            // Toggle state based on melee or not.
            if(state->Gamepad.wButtons & XINPUT_GAMEPAD_X) {
                if(XDown == false) {
                    XDown = true;
                    m_TimeStamp = std::time(0);
                    m_XPressed = true;
                }
            } else {
                if(XDown == true) {
                    XDown = false;
                }
            }
            
            // Check if in conversation and if so, go to 3rd person.
            if(state->Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
                if(FirstPerson == true) {
                    const auto DM = DialogueManager::get_instance();
                    if(DM) {
                        IsInDialogue = DM->is_in_dialogue();
                        if(IsInDialogue) {
                           KeyDown = true;
                           input.ki.dwFlags = 0;
                           SendInput(1, &input, sizeof(INPUT));
                           FirstPerson = false;
                           Cause = CAUSE_DIALOGUE;
                           API::get()->log_info("In Dialogue=%d. Switching to 3rd person.", IsInDialogue);
                        }
                    } 
                }
            }


            // Check if in cinematic (movie) and if so, go to 3rd person.
            if(FirstPerson == true) {
                const auto CM = CinematicManager::get_instance();
                if(CM) {
                    IsInCinematic = CM->is_in_cinematic();
                    if(IsInCinematic) {
                       KeyDown = true;
                       input.ki.dwFlags = 0;
                       SendInput(1, &input, sizeof(INPUT));
                       FirstPerson = false;
                       Cause = CAUSE_CINEMATIC;
                       API::get()->log_info("In Cinematic=%d. Switching to 3rd Person", IsInCinematic);
                    }
                } 
            }
                
                
        }

    }
};
    
// Actually creates the plugin. Very important that this global is created.
// The fact that it's using std::unique_ptr is not important, as long as the constructor is called in some way.
std::unique_ptr<UevrPlugin> g_plugin{new UevrPlugin()};

void DebugPrint(char* Format, ...)
{
  char FormattedMessage[512];    
  va_list ArgPtr = NULL;  
  
  /* Generate the formatted debug message. */        
  va_start(ArgPtr, Format);        
  vsprintf(FormattedMessage, Format, ArgPtr);        
  va_end(ArgPtr); 

  OutputDebugString(FormattedMessage);
}

