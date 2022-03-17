#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>

struct CacheBlock {
    bool valid = false;
    int tag = 0;
    int time = 0;
};

inline int getCacheBlockIndex(const int setIndex, const int blockIndex, const int setDegree) {
    return setIndex * setDegree + blockIndex;
}

int main(int argc, char **argv) {
    // Load the arguments
    if (argc > 5) {
        std::cerr << "Too many arguments!" << std::endl;
        exit(1);
    } else if (argc < 5) {
        std::cerr << "Missing arguments!" << std::endl;
        exit(1);
    }
    std::string traceFilePath = argv[1];
    int cacheSize = std::strtol(argv[2], nullptr, 10);
    int blockSize = std::strtol(argv[3], nullptr, 10);
    int setDegree = std::strtol(argv[4], nullptr, 10);
    if (!cacheSize || !blockSize || !setDegree) {
        std::cerr << "Arguments format incorrect!" << std::endl;
        exit(1);
    }

    // Read the trace file
    std::fstream traceFile;
    traceFile.open(traceFilePath, std::ios::in);
    if (!traceFile) {
        std::cerr << "Cannot open the trace file!" << std::endl;
        exit(1);
    }

    // Count the number of cache blocks. note: 1024 stands for 'K'Byte, 4 stands for 1word = 4bytes
    int blockCount = (cacheSize * 1024) / (blockSize * 4);
    // Count the number of sets
    int setCount = blockCount / setDegree;
    if (setCount <= 0) {
        std::cerr << "Set degree cannot be larger than the number of cache blocks" << std::endl;
        exit(1);
    }

    // std::cout << "BlockCount: " << blockCount << "\nSetCount: " << setCount << std::endl;

    // Allocate memory space for the cache
    auto *cache = new CacheBlock[blockCount];

    // Counters
    int counter = 0, hitCounter = 0;

    // Read the trace data
    while (!traceFile.eof()) {
        char memoryAddress[11];
        traceFile.getline(memoryAddress, sizeof(memoryAddress));
        // Get the memory block index in which the address is located
        int memoryBlockIndex = std::strtol(memoryAddress, nullptr, 16) / (blockSize * 4);
        int setIndex = memoryBlockIndex % setCount;
        int tag = floor((double) memoryBlockIndex / setCount);

        // Check if the cache block hit or not
        bool hit = false;
        // Find any empty blocks (valid flag = false) and get its index. If not found -> return -1
        int emptyIndex = -1;
        // Get the least recently used block in the set
        // Take first block as initial value, replace blocks if any block has the least counter value
        int lruIndex = getCacheBlockIndex(setIndex, setDegree - 1, setDegree);
        // Loop from the back, so that empty block in the front can be used first
        for (int i = setDegree - 1; i >= 0; i--) {
            int index = getCacheBlockIndex(setIndex, i, setDegree);
            if (cache[index].valid && cache[index].tag == tag) { // Hit
                std::cout << std::setw(5) << counter + 1 << " [Hit!] ";
                hit = true;
                hitCounter++;
                break;
            }
            if (!cache[index].valid) { // Find an empty block
                emptyIndex = index;
            }
            if (cache[index].time < cache[lruIndex].time) { // Find LRU block
                lruIndex = index;
            }
        }
        if (!hit) { // Miss
            std::cout << std::setw(5) << counter + 1 << " [Miss] ";
            if (emptyIndex != -1) { // Insert into an empty block
                cache[emptyIndex].valid = true;
                cache[emptyIndex].tag = tag;
                cache[emptyIndex].time = counter;
            } else { // No empty block -> Remove LRU block then insert new
                cache[lruIndex].tag = tag;
                cache[lruIndex].time = counter;
            }
        }
        std::cout << memoryAddress << " -> "
                  << std::setw(10) << memoryBlockIndex << " -> "
                  << std::setw(5) << setIndex << " -> "
                  << std::setw(10) << tag << std::endl;
        counter++;
    }

    std::cout << "\nTotal: " << counter << " / Hit: " << hitCounter << " / Miss: " << counter - hitCounter << std::endl;
    std::cout << "MissRate: " << (double) (counter - hitCounter) / counter << std::endl;

    traceFile.close();
    delete[] cache;
    return 0;
}
