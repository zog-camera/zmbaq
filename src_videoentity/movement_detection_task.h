#ifndef MOVEMENTDETECTIONTASK_H
#define MOVEMENTDETECTIONTASK_H

#include <functional>
#include <memory>
#include "../src/mimage.h"

namespace ZMBEntities {

class MovementDetectionTask
{
public:
    MovementDetectionTask(const std::string& taskName);
    
    std::string name;
};

}//ZMBEntities

#endif // MOVEMENTDETECTIONTASK_H
