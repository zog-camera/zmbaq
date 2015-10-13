#ifndef MOVEMENTDETECTIONTASK_H
#define MOVEMENTDETECTIONTASK_H

#include <functional>
#include <memory>
#include <Poco/Task.h>
#include "src/mimage.h"

namespace ZMBEntities {

class MovementDetectionTask : public Poco::Task
{
public:
    MovementDetectionTask(const std::string& name);
    
    
};

}//ZMBEntities

#endif // MOVEMENTDETECTIONTASK_H
