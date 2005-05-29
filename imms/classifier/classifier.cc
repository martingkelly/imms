#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <map>

#include <immsconf.h>
#include <appname.h>
#include <immsutil.h>
#include <song.h>
#include <immsdb.h>

#include <analyzer/beatkeeper.h>

#include "model.h"
#include "distance.h"

using std::string;
using std::ifstream;
using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::map;

const string AppName = CLASSIFIER_APP;

void usage()
{
    cerr << "Usage: " << AppName << " --model=<name> "
        << "--positive=<file|dir> --negative=<file|dir>" << endl;
    exit(-1);
}

void build_feature_vector(int uid1, int uid2, vector<float> &features)
{
    Song song1("", uid1), song2("", uid2);

    MixtureModel m1, m2;
    float beats1[BEATSSIZE];
    float beats2[BEATSSIZE];

    if (!song1.get_acoustic(&m1, sizeof(MixtureModel),
                beats1, sizeof(beats2)))
    {
        cerr << "warning: failed to load cepstrum data uid " << uid1 << endl;
        return;
    }

    if (!song2.get_acoustic(&m2, sizeof(MixtureModel),
                beats2, sizeof(beats2)))
    {
        cerr << "warning: failed to load cepstrum data uid " << uid2 << endl;
        return;
    }

    features.push_back(EMD::distance(m1, m2));

    BeatKeeper::extract_features(beats1, features);
    BeatKeeper::extract_features(beats2, features);
}

bool load_examples_from_file(const string &path, list<string> &files)
{
    ifstream in(path.c_str());

    string line;
    while (getline(in, line))
        files.push_back(line);

    return true;
}

bool load_examples_from_dir(const string &path, list<string> &files)
{
    LOG(ERROR) << "reading from directories not implemented yet!" << endl;
    return false;
}

bool mass_identify(const list<string> &files,
        map<int, int> &data, int type)
{
    for (list<string>::const_iterator i = files.begin();
            i != files.end(); ++i)
    {
        Song song(*i);

        if (!song.isok())
        {
            LOG(ERROR) << "failed to identify " << *i << endl;
            return false;
        }

        if (!song.isanalyzed())
            LOG(ERROR) << "warning: song not analyzed: " << *i << endl;

        cout << ".";
        cout.flush();
        data[song.get_uid()] = type;
    }

    return true;
}

bool load_examples(const string &path, list<string> &files)
{
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf))
    {
        LOG(ERROR) << "could not stat " << path << endl;
        return false;
    }

    if (S_ISREG(statbuf.st_mode))
        return load_examples_from_file(path, files);
    if (S_ISDIR(statbuf.st_mode))
        return load_examples_from_dir(path, files);

    LOG(ERROR) << path << " is neither a file nor directory!" << endl;
    return false;
}

int main(int argc, char *argv[])
{
    string name, positive, negative;

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", 0, 0, 'h'},
            {"model", 1, 0, 'm'},
            {"positive", 1, 0, 'p'},
            {"negative", 1, 0, 'n'},
            {0, 0, 0, 0}
        };

        int opt = getopt_long(argc, argv, "hm:p:n:", long_options,
                &option_index);

        if (opt == -1)
            break;

        if (opt == '?' || opt == ':')
            usage();

        switch (opt)
        {
            case 'm':
                name = optarg; 
                break;
            case 'p':
                positive = optarg; 
                break;
            case 'n':
                negative = optarg; 
                break;
            case 'h':
                usage();
            default:
                break;
        }
    }

    while (optind < argc)
    {
        if (name == "")
            name = argv[optind++];
        else if (positive == "")
            positive = argv[optind++];
        else if (negative == "")
            negative = argv[optind++];
        else
            usage();
    }

    if (name == "" || positive == "" || negative == "")
        usage();

    list<string> positive_samples, negative_samples;
    if (!load_examples(positive, positive_samples)
            || !load_examples(negative, negative_samples))
    {
        LOG(ERROR) << "failed to load example lists!" << endl;
        return -1;
    }

    ImmsDb immsdb;

    LOG(INFO) << "Identifying files: ";
    cout.flush();
    map<int, int> data;
    if (!mass_identify(positive_samples, data, 1)
            || !mass_identify(negative_samples, data, -1))
    {
        LOG(INFO) << endl;
        LOG(ERROR) << "failed to identify some examples!" << endl;
        return -2;
    }
    cout << endl;

    LOG(INFO) << "successfully identified all files" << endl;

    Model model(name);
    float accuracy = model.train(data);
    DEBUGVAL(accuracy);

    return 0;
}
