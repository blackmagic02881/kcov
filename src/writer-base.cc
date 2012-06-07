#include "writer-base.hh"
#include <utils.hh>
#include <filter.hh>

#include <stdio.h>

using namespace kcov;

struct summaryStruct
{
	uint32_t nLines;
	uint32_t nExecutedLines;
	char name[256];
};

WriterBase::WriterBase(IElf &elf, IReporter &reporter, IOutputHandler &output) :
		m_elf(elf), m_reporter(reporter)
{
		m_elf.registerListener(*this);
}

WriterBase::File::File(const char *filename) :
						m_name(filename), m_codeLines(0), m_executedLines(0), m_lastLineNr(0)
{
	size_t pos = m_name.rfind('/');

	if (pos != std::string::npos)
		m_fileName = m_name.substr(pos + 1, std::string::npos);
	else
		m_fileName = m_name;
	m_outFileName = m_fileName + ".html";

	readFile(filename);
}

void WriterBase::File::readFile(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	unsigned int lineNr = 1;

	panic_if(!fp, "Can't open %s", filename);

	while (1)
	{
		char *lineptr = NULL;
		ssize_t res;
		size_t n;

		res = getline(&lineptr, &n, fp);
		if (res < 0)
			break;
		m_lineMap[lineNr] = std::string(lineptr);

		free((void *)lineptr);
		lineNr++;
	}

	m_lastLineNr = lineNr;

	fclose(fp);
}


void WriterBase::onLine(const char *file, unsigned int lineNr, unsigned long addr)
{
	m_fileMutex.lock();

	if (m_files.find(std::string(file)) != m_files.end())
		goto out;

	if (!file_exists(file))
		goto out;

	m_files[std::string(file)] = new File(file);

out:
	m_fileMutex.unlock();
}


void *WriterBase::marshalSummary(IReporter::ExecutionSummary &summary,
		std::string &name, size_t *sz)
{
	struct summaryStruct *p;

	p = (struct summaryStruct *)xmalloc(sizeof(struct summaryStruct));
	memset(p, 0, sizeof(*p));

	p->nLines = summary.m_lines;
	p->nExecutedLines = summary.m_executedLines;
	strncpy(p->name, name.c_str(), sizeof(p->name) - 1);

	*sz = sizeof(*p);

	return (void *)p;
}

bool WriterBase::unMarshalSummary(void *data, size_t sz,
		IReporter::ExecutionSummary &summary,
		std::string &name)
{
	struct summaryStruct *p = (struct summaryStruct *)data;

	if (sz != sizeof(*p))
		return false;

	summary.m_lines = p->nLines;
	summary.m_executedLines = p->nExecutedLines;
	name = std::string(p->name);

	return true;
}