# 370CT_Dining_Philosopher
A semaphore solution to synchronize the concurrent activities of the three men who are sitting on a circular table, with 3 soldering irons, one to the left and one to the right. To do the soldering, they need both left and right soldering irons. Those that don’t have the soldering iron will have to wait until both are available. 

## How to run this program? 
1. Run `g++ -std=c++17 solution.cpp -o solution.out` to compile the program.
2. Run `./solution.out <Number of workers>` to execute the program. Replace `Number of workers` with the actual number of workers.
