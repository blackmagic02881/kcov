#include <configuration.hh>
#include <engine.hh>
#include <reporter.hh>
#include <writer.hh>
#include <collector.hh>
#include <output-handler.hh>
#include <elf.hh>

#include <string.h>
#include <signal.h>

#include "html-writer.hh"
#include "cobertura-writer.hh"

using namespace kcov;

static IOutputHandler *g_output;

static void ctrlc(int sig)
{
	g_output->stop();
	IEngine::getInstance().kill();
	exit(0);
}


int main(int argc, const char *argv[])
{
	IConfiguration &conf = IConfiguration::getInstance();

	if (!conf.parse(argc, argv))
		return 1;

	std::string file = conf.getBinaryPath() + conf.getBinaryName();
	IElf *elf = IElf::open(file.c_str());
	if (!elf) {
		conf.printUsage();
		return 1;
	}

	ICollector &collector = ICollector::create(elf);
	IReporter &reporter = IReporter::create(*elf, collector);
	IOutputHandler &output = IOutputHandler::create(reporter);

	// Register writers
	IWriter &htmlWriter = createHtmlWriter(*elf, reporter, output);
	IWriter &coberturaWriter = createCoberturaWriter(*elf, reporter, output);
	output.registerWriter(htmlWriter);
	output.registerWriter(coberturaWriter);

	g_output = &output;
	signal(SIGINT, ctrlc);
	signal(SIGTERM, ctrlc);

	output.start();
	int ret = collector.run();
	output.stop();
	IEngine::getInstance().kill();

	return ret;
}
