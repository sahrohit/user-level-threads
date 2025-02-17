#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

// Creating shorthand for itimerval struct
struct itimerval it;

// Handler when the virtual timer expires
void signal_handler()
{
  // Static to preserve the value of i from being re-assigned
  static int i = 0;
  i += 1;
  printf("(%2d) handled a timer signal, setting timer\n", i);

  // Setting the timer to 500ms
  // This calls SIGVTALRM signal after 500ms
  // Hence this again calls this signal_handler function
  setitimer(ITIMER_VIRTUAL, &it, NULL);
}

int main()
{

  // Registering the signal handler for SIGVTALRM
  signal(SIGVTALRM, signal_handler);

  // Setting the timer to 500ms
  it.it_value.tv_sec = 1;
  it.it_value.tv_usec = 500000; // 500ms

  setitimer(ITIMER_VIRTUAL, &it, NULL);

  // This loop is for preventing the program from exiting
  while (1)
  {
    // printf("main loop\n");
  }

  return 0;
}
