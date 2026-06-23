#include <windows.h>

#include "logger.hpp"
#include <corelib.hpp>
#include <standardpaths.hpp>

#ifndef SERVICE_ACCEPT_USERMODEREBOOT
#define SERVICE_ACCEPT_USERMODEREBOOT 0x00000800
#endif

const char *SERVICE_NAME = "Synqueen Service";
SERVICE_STATUS_HANDLE statusHandle = nullptr;

VOID reportServiceStatus(DWORD state, DWORD exitCode) {
  static DWORD checkPoint = 1;

  SERVICE_STATUS status = {};
  status.dwServiceType = SERVICE_USER_OWN_PROCESS;
  status.dwCurrentState = state;
  status.dwWin32ExitCode = exitCode;

  if (state == SERVICE_RUNNING)
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                SERVICE_ACCEPT_PRESHUTDOWN |
                                SERVICE_ACCEPT_USERMODEREBOOT;

  if (state == SERVICE_RUNNING || state == SERVICE_STOPPED)
    status.dwCheckPoint = 0;
  else
    status.dwCheckPoint = checkPoint++;

  // Report the status of the service to the SCM
  SetServiceStatus(statusHandle, &status);
}

void initializeService() {
  synqueen::StandardPaths::initialize("Synqueen");
  auto logger = synqueen::createLogger();
  spdlog::set_default_logger(logger);
  synqueen::initialize(logger);
}

void runService() { synqueen::run(); }

void WINAPI serviceCtrlHandler(DWORD ctrlCode) {
  switch (ctrlCode) {
  case SERVICE_CONTROL_STOP:
    reportServiceStatus(SERVICE_STOP_PENDING, 0);
    synqueen::stop();
    reportServiceStatus(SERVICE_STOPPED, 0);
    break;
  }
}

void WINAPI serviceMain(DWORD argc, LPSTR *argv) {
  statusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, serviceCtrlHandler);

  SERVICE_STATUS status = {};

  status.dwCurrentState = SERVICE_START_PENDING;
  SetServiceStatus(statusHandle, &status);
  initializeService();

  status.dwServiceType = SERVICE_USER_OWN_PROCESS;
  status.dwCurrentState = SERVICE_RUNNING;
  status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PRESHUTDOWN |
                              SERVICE_ACCEPT_USERMODEREBOOT;
  SetServiceStatus(statusHandle, &status);

  runService();
}

int main(int argc, char *argv[]) {
  if (argc > 1 && strcmp(argv[1], "--app") == 0) {
    // Run as an application
    initializeService();
    runService();
    return 0;
  }

  SERVICE_TABLE_ENTRY serviceTable[] = {{(LPSTR)SERVICE_NAME, serviceMain},
                                        {nullptr, nullptr}};

  StartServiceCtrlDispatcher(serviceTable);
  return 0;
}
