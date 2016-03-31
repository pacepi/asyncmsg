# asyncmsg
it is a high-perfomance mutual exclusion queue. to make communication easier and faster.  

  
#HOWTO :  
[build]  
1, download code.  
2, cd asyncmsg  
3, source setupenv  
4, make  
  
[run]  
1, cd asyncmsg  
2, asyncmsg -h[for help]  
3, edit input.conf and output.conf as you need  
  
[hot restart]  
you can send signals to asyncmsg, asyncmsg will reload config file immediately.  
SIGUSR1 : reload input config file. Ex, kill -10 PID.  
SIGUSR2 : reload output config file. Ex, kill -12 PID.  
SIGCHLD : show current status. Ex, kill -17 PID.  
