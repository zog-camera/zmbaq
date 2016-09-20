#include "av_things.h"
#include "videoentity.h"
#include "videoentity_pv.h"
#include "survcamparams.h"

#include <cstdlib>
#include <unistd.h>

int main(int argc, char** argv)
{
  Json::Value config;
  config[ZMB::SURV_CAM_PARAMS_PATH.begin()] = argv[1];
  config[ZMB::SURV_CAM_PARAMS_NAME.begin()] = "camera0";

  ZMBEntities::VideoEntity ve;
  assert(ve.is_cfg_viable(&config));

  ZMBEntities::VEMp4WritingVisitor visitor;
  visitor.visit(&ve, &config);

  ::sleep(4);

  ZMBEntities::VECleanupVisitor cleaner;
  cleaner.visit(&ve);


  return 0;
}
