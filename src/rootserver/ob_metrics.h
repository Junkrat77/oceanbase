#ifndef OCEANBASE_ROOTSERVER_OB_METRICS_H
#define OCEANBASE_ROOTSERVER_OB_METRICS_H
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>
#include <prometheus/histogram.h>
#include <unordered_map>

using namespace prometheus;

namespace oceanbase{
namespace rootserver{

class Desc{
private:
	/* data */
	std::string name;
	std::string help;
	uint64_t type;
	Labels labels; 
public:
	explicit Desc(uint64_t type, std::string name, std::string help, Labels labels) {
		this->type = type;
		this->name = name;
		this->help = help;
		this->labels = labels; // 深拷贝?
	};


	explicit Desc(uint64_t type, std::string name, std::string help) {
		this->type = type;
		this->name = name;
		this->help = help;
		labels = {};
	};

	uint64_t get_type() {
		return type;
	}

	std::string get_name() {
		return name;
	};

	std::string get_help() {
		return help;
	};

	Labels get_labels() {
		return labels;
	};

};

enum MatricType {
	METRIC_TYPE_COUNTER,
	METRIC_TYPE_GAUGE,
	METRIC_TYPE_SUMMARY,
	METRIC_TYPE_UNTYPED,
	METRIC_TYPE_HISTOGRAM
};

class Metric {
private:
	uint64_t type;
	void* instant;
public:
	explicit Metric() {
		type = METRIC_TYPE_UNTYPED;
		instant = 0;
	};

	explicit Metric(uint64_t type, void* metric): type(type), instant(metric) {
	};
	
	uint64_t get_type() {
		return type;
	}
	void* get_metric() {
		return instant;
	}

};

class ObMetrics
{
private:
	/* data */
	bool is_init = false;
	Exposer exposer;
	std::shared_ptr<Registry> registry;
	std::unordered_map<int, Metric*> ob_metrics; // TODO: use ObIArray

public:
	ObMetrics();

	explicit ObMetrics(std::string ip_port): exposer(ip_port) {
	};

	~ObMetrics();

	// init all metric
	int ob_metrics_init();

	int ob_metrics_increment(uint64_t id);

	int ob_metrics_increment(uint64_t id, double value);

	int ob_metrics_decrement(uint64_t id) ;

	int ob_metrics_decrement(uint64_t id, double value);

	int ob_metrics_gauge_set(uint64_t id, double value);

    int ob_metrics_gauge_set_to_current(uint64_t id);
	//todo : add observer

};

}
}

#endif	// 

// todo: record all metric?