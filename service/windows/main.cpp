#include <windows.h>

#ifndef SERVICE_ACCEPT_USERMODEREBOOT
#define SERVICE_ACCEPT_USERMODEREBOOT 0x00000800
#endif

const char *SERVICE_NAME = "Synqueen Service";
SERVICE_STATUS g_ServiceStatus = {};
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;

void RunApplication() {
  // Your application logic here
  while (true) {
    Sleep(1000);
    // Do some work
  }
}

void WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
  switch (ctrlCode) {
  case SERVICE_CONTROL_STOP:
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
    break;
  }
}

void WINAPI ServiceMain(DWORD argc, LPSTR *argv) {
  g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

  g_ServiceStatus.dwServiceType = SERVICE_USER_OWN_PROCESS;
  g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
  g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_PRESHUTDOWN |
                                       SERVICE_ACCEPT_USERMODEREBOOT;
  SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

  g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
  SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

  while (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
    Sleep(1000);

    // Your service work here
  }
}

int main(int argc, char *argv[]) {
  if (argc > 1 && strcmp(argv[1], "--app") == 0) {
    // Run as an application
    RunApplication();
    return 0;
  }

  SERVICE_TABLE_ENTRY serviceTable[] = {{(LPSTR)SERVICE_NAME, ServiceMain},
                                        {nullptr, nullptr}};

  StartServiceCtrlDispatcher(serviceTable);
  return 0;
}
