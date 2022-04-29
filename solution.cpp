#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <chrono>
#include <initializer_list>
#include <list>
#include <set>
#include <cstdlib>
#include <ctime>   

/// Author : Tan Hoe Theng
/// # 100% original work

class Worker {
 private:
    /// Worker's required soldering iron
    std::vector<int> solIronsReq;  
    /// Worker's id
    int id;  

 public:

    /// Construct a worker with [id] and [solIronsReq] 
    Worker(int id, const std::vector<int> &solIronsReq)
        : id(id), solIronsReq(solIronsReq) {
    }

    /// The worker's status
    enum Status {
        Soldering,
        Thinking,
        Putting
    };

    /// The worker's id
    int getId() const {
        return id;
    }

    /// The worker's required soldering irons
    std::vector<int> getRequiredSolIron() const {
        return solIronsReq;
    }

    /// Print the worker's [status]
    void printStatus(Status status) const {
        switch (status) {
        case Soldering:
            printf("Worker %d is soldering\n", id);
            break;
        case Thinking:
            printf("Worker %d is thinking\n", id);
            break;
        case Putting:
            printf("Worker %d is putting down the soldering iron\n", id);
            break;
        default:
            printf("Unknown behavior\n");
        }
    }
};

class Supervisor {

    /// The state of soldering iron
    enum SolIronState {
        /// The soldering iron is not used by any worker
        Free,
        /// The soldering iron is allocated to a worker
        Allocated,
    };

    /// A mutually exclusive flag to ensure only one thread access to the 
    /// critical region
    std::mutex mtx;

    /// A monitor to inform workers that the soldering iron is available
    std::vector<std::condition_variable> conds;

    /// A queue storing all the blocked workers that wait for soldering irons
    std::list<Worker *> blockedQueue;

    /// Soldering irons that required by workers
    std::vector<SolIronState> solderingIrons;

    /// Process the workers in [blockedQueue]
    ///
    /// The first worker in the queue is the most starved worker. Thus,
    /// the supervisor should allocate soldering iron to him whenever the 
    /// soldering iron required is released by other worker. While the first 
    /// worker is waiting for the soldering iron, the other workers whose 
    /// soldering iron is not needed by the first worker should continue running
    /// to maximize the throughput (CPU Usage). This is to ensure fairness and 
    /// prevent starvation while maximize the CPU usage.
    void processBlockedQueue() {

        /// Exit if no worker in the [blockedQueue]
        if (blockedQueue.empty())
            return;
        
        /// The first worker in the [blockedQueue]
        std::list<Worker *>::iterator it = blockedQueue.begin();
        const Worker *const firstWorker = *it;
        const std::vector<int>& solIrons = firstWorker->getRequiredSolIron();

        /// Store the soldering iron booked by the [firstWorker]
        std::set<int> solIronBooked;

        /// If the [firstWorker] cannot get the soldering iron required, records 
        /// them in [solIronBooked] 
        if (!allocSolIron(firstWorker)) {
            solIronBooked.insert(solIrons.begin(), solIrons.end());
            it++;
        } else {
            it = blockedQueue.erase(it);
        }

        /// Starting from the next worker in the [blockedQueue], allocate
        /// the soldering iron to the worker, if the soldering iron
        /// is avaialble and not booked by the first worker
        while (it != blockedQueue.end()) {
            Worker *worker = *it;
            const std::vector<int>& solIrons = worker->getRequiredSolIron();

            bool isReserved = false;
            for (int i : solIrons) 
                if (solIronBooked.count(i)) 
                    isReserved = true;

            if (!isReserved && allocSolIron(worker)) 
                it = blockedQueue.erase(it);
            else 
                it++;
        }

    }

    /// Allocate soldering irons to the [worker]
    ///
    /// Return false if the [worker]'s soldering irons are inavailable; 
    /// Return true if the soldering irons are allocated to the [worker]
    bool allocSolIron(const Worker *const worker) {
        const std::vector<int>& solIrons = worker->getRequiredSolIron();

        if (!isSolIronAvail(solIrons))
            return false;

        for (int i : solIrons)
            solderingIrons[i] = Allocated;
        
        conds[worker->getId()].notify_all();
        return true;
    }

    /// Check whether the [solIrons] is avaialble
    bool isSolIronAvail(const std::vector<int> &solIrons) {
        for (int i : solIrons)
            if (solderingIrons[i] == Allocated)
                return false;
        return true;
    }

 public:
    /// Construct Supervisor with [numOfSolIrons] soldering irons
    Supervisor(int numOfSolIrons) : 
        conds(numOfSolIrons), 
        solderingIrons(numOfSolIrons, Free) {
    }

    /// Release the [worker]'s soldering iron
    ///
    /// Process the [blockedQueue] after releasing the the soldering irons.
    void relSolIron(Worker *worker) {
        std::unique_lock<std::mutex> lock(mtx);
        const std::vector<int> &res = worker->getRequiredSolIron();

        for (int i : res)
            solderingIrons[i] = Free;

        processBlockedQueue();
    }

    /// Request [worker]'s soldering irons
    ///
    /// Block and push the [worker] to the [blockedQueue] if the soldering irons
    /// are inavailable.
    void reqSolIron(Worker *worker) {
        std::unique_lock<std::mutex> lock(mtx);

        /// If [blockedQueue] is empty, this worker can use soldering iron if 
        /// the required soldering iron is available. 
        /// If [blockedQueue] is not empty, this worker will be added to
        /// [blockedQueue] to ensure fairness
        if (!(blockedQueue.empty() && allocSolIron(worker))) {
            blockedQueue.push_back(worker);
            conds[worker->getId()].wait(lock);
        }
    }
};

void startWorking(Supervisor *, Worker *);

int main(int argc, char** argv) {
    if (argc < 2)
        return -1;

    /// The number of worker
    const int NUM_OF_WORKER = atoi(argv[1]);

    /// Initialize random seed
    srand(time(NULL));
    
    /// Vector of threads
    std::vector<std::thread> threads;

    /// Vector of workers
    std::vector<Worker> workers;

    /// Supervisor with [Num_OF_WORKER] soldering irons
    Supervisor supervisor(NUM_OF_WORKER);

    /// Construct and insert worker object to the [workers]
    for (int i = 0; i < NUM_OF_WORKER; i++)
        workers.emplace_back(
            i, std::initializer_list<int>{i, (i + 1) % NUM_OF_WORKER});

    /// Create and insert threads to [threads]
    for (int i = 0; i < NUM_OF_WORKER; i++)
        threads.emplace_back(startWorking, &supervisor, &(workers[i]));

    /// Synchronize the threads in [threads]
    for (int i = 0; i < NUM_OF_WORKER; i++)
        threads[i].join();

    return 0;
}

/// Simulate [worker] actions, including [thinking], [soldering], and [putting]
void startWorking(Supervisor *supervisor, Worker *worker) {
    /// The duration to sleep a thread
    const int DURATION = 3;

    /// Simulate infinity times
    while (true) {
        worker->printStatus(Worker::Thinking);
        std::this_thread::sleep_for(
            std ::chrono::seconds(rand() % DURATION + 1));

        supervisor->reqSolIron(worker);

        worker->printStatus(Worker::Soldering);
        std::this_thread::sleep_for(
            std ::chrono::seconds(rand() % DURATION + 1));

        worker->printStatus(Worker::Putting);
        std::this_thread::sleep_for(
            std ::chrono::seconds(rand() % DURATION + 1));

        supervisor->relSolIron(worker);
    }
}
