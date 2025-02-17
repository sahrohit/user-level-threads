#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

struct itimerval it;

void signal_handler()
{
  static int i = 0;
  i += 1;
  printf("(%2d) handled a timer signal, setting timer\n", i);
  setitimer(ITIMER_VIRTUAL, &it, NULL);
}

int main()
{
  signal(SIGVTALRM, signal_handler);
  it.it_value.tv_sec = 0;
  it.it_value.tv_usec = 500000; // 500ms
  setitimer(ITIMER_VIRTUAL, &it, NULL);
  while (1)
  {
    // printf("main loop\n");
  }
}
