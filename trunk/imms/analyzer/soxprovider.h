#include <sstream>
#include <string>

#include <immsutil.h>
#include <strmanip.h>

using std::string;
using std::endl;
using std::ostringstream;

FILE *run_sox(const string &path, int samplerate, bool sign = false)
{
    string epath = rex.replace(path, "'", "'\"'\"'", Regexx::global);
    string extension = string_tolower(path_get_extension(path));

    ostringstream command;
    command << "nice -n 15 sox ";
    if (extension == "mp3" || extension == "ogg")
        command << "-t ." << extension << " ";
    command << "\'" << epath << "\' ";
    command << "-t .raw -w -c 1 -p -r " << samplerate
        << (sign ? " -s" : " -u") << " -";
    LOG(INFO) << "Executing: " << command.str() << endl;
    return popen(command.str().c_str(), "r");
}
