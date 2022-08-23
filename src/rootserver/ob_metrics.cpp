#include "ob_metrics.h"

// #include "ob_major_freeze_launcher.h"

// #include "share/ob_debug_sync.h"
// #include "share/ob_common_rpc_proxy.h"
// #include "share/config/ob_server_config.h"
// #include "rootserver/ob_freeze_info_manager.h"
#include "rootserver/ob_root_service.h"
#define USING_LOG_PREFIX RS
using namespace prometheus;

namespace oceanbase {

using namespace common;
using namespace share;
namespace rootserver {

// todo: 这个的生成我觉得可以在bootstrap的时候做对应的select语句, 然后自动生成,我觉得可行(不太可行其实)
// 只能在这里边加metric
Desc metrics_descs[] = {
    // show global status
    Desc(METRIC_TYPE_GAUGE, "ob_global_status_threads_connected",
        "The total number of threads connected to this observer"),
    Desc(METRIC_TYPE_GAUGE, "ob_global_status_uptime", "The observer uptime"),
    //
    Desc(METRIC_TYPE_UNTYPED, "", "", {})};

enum counter_metric_ids {
  OB_METRICS_1,
  OB_METRICS_2,
};

uint64_t metrics_num = 2;


ObMetrics::ObMetrics(): exposer("11.124.9.227:3456") {
  registry = std::make_shared<Registry>();
  LOG_INFO("[gq] init registry");
};

int ObMetrics::ob_metrics_init()
{
  if(is_init) {
    LOG_INFO("[gq] ObMetrics already init");
    return 1;
  }
  is_init = true;
  // for (int i = 0; i < metrics_num; i++) {
  //   if (metrics_descs[i].get_type() == METRIC_TYPE_COUNTER) {
  //     Family<Counter> &family =
  //         BuildCounter().Name(metrics_descs[i].get_name()).Help(metrics_descs[i].get_help()).Register(*registry);
  //     Counter &counter = family.Add(metrics_descs[i].get_labels());
  //     ob_metrics[i] = new Metric(METRIC_TYPE_COUNTER, &counter);
  //   } else if (metrics_descs[i].get_type() == METRIC_TYPE_GAUGE) {
  //     Family<Gauge> &family =
  //         BuildGauge().Name(metrics_descs[i].get_name()).Help(metrics_descs[i].get_help()).Register(*registry);
  //     Gauge &gauge = family.Add(metrics_descs[i].get_labels());
  //     ob_metrics[i] = new Metric(METRIC_TYPE_GAUGE, &gauge);
  //   }
  //   // todo: add summary
  //   else {
  //     exit(-1);  // not support
  //   }
  // }
  exposer.RegisterCollectable(registry);
  LOG_INFO("[gq]: Collector registey successfully");
  return 1;
}

int ObMetrics::ob_metrics_increment(uint64_t id)
{
  LOG_INFO("[gq] ob_metrics_increment", K(id));
  uint64_t metric_type = ob_metrics[id]->get_type();
  if (metric_type == METRIC_TYPE_COUNTER) {
    Counter *counter = static_cast<Counter *>(ob_metrics[id]->get_metric());
    counter->Increment();
  } else if (metric_type == METRIC_TYPE_GAUGE) {
    Gauge *gauge = static_cast<Gauge *>(ob_metrics[id]->get_metric());
    gauge->Increment();
  } else {
    exit(0);
  }
  return 1;
}

int ObMetrics::ob_metrics_increment(uint64_t id, double value)
{
  uint64_t metric_type = ob_metrics[id]->get_type();
  if (metric_type == METRIC_TYPE_COUNTER) {
    Counter *counter = static_cast<Counter *>(ob_metrics[id]->get_metric());
    counter->Increment(value);
  } else if (metric_type == METRIC_TYPE_GAUGE) {
    Gauge *gauge = static_cast<Gauge *>(ob_metrics[id]->get_metric());
    gauge->Increment(value);
  } else {
    exit(0);
  }
  return 1;
}

int ObMetrics::ob_metrics_decrement(uint64_t id)
{
  uint64_t metric_type = ob_metrics[id]->get_type();
  if (metric_type == METRIC_TYPE_GAUGE) {
    Gauge *gauge = static_cast<Gauge *>(ob_metrics[id]->get_metric());
    gauge->Decrement();
  } else {
    exit(0);
  }
  return 1;
}

int ObMetrics::ob_metrics_decrement(uint64_t id, double value)
{
  uint64_t metric_type = ob_metrics[id]->get_type();
  if (metric_type == METRIC_TYPE_GAUGE) {
    Gauge *gauge = static_cast<Gauge *>(ob_metrics[id]->get_metric());
    gauge->Decrement(value);
  } else {
    exit(0);
  }
  return 1;
}

int ObMetrics::ob_metrics_gauge_set(uint64_t id, double value)
{
  uint64_t metric_type = ob_metrics[id]->get_type();
  if (metric_type == METRIC_TYPE_GAUGE) {
    Gauge *gauge = static_cast<Gauge *>(ob_metrics[id]->get_metric());
    gauge->Decrement(value);
  } else {
    exit(0);
  }
  return 1;
}

int ObMetrics::ob_metrics_gauge_set_to_current(uint64_t id)
{
  uint64_t metric_type = ob_metrics[id]->get_type();
  if (metric_type == METRIC_TYPE_GAUGE) {
    Gauge *gauge = static_cast<Gauge *>(ob_metrics[id]->get_metric());
    gauge->SetToCurrentTime();
  } else {
    exit(0);
  }
  return 1;
}

ObMetrics::~ObMetrics() {
  LOG_INFO("[gq] ~ObMetrics");
}
// todo : add observer
}
}  // namespace oceanbase

// todo: record all metric?