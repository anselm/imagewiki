/*

*/

#include "ImageDB.hpp"

#include <iostream>
#include <errno.h>

#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "sift.h"
}

void usage(const char* progname)
{
    std::cerr << progname << " queries a database for matches against an image.\n\n";
    std::cerr << "Usage: " << progname << " [-e] <db path> <image path>\n";
}

typedef std::list<std::string> StringList;
typedef std::vector<MatchResult> ResultVector;



bool result_score_cmp(const MatchResult& r1, const MatchResult& r2)
{
    return r1.score > r2.score;
}


int do_match(ImageDB *db, const std::string& path, int max_nn_chks, bool exhaustive_match)
{
    clock_t start = clock();

    IplImage *image = cvLoadImage(path.c_str(), 1);
    if (!image) {
        std::cerr << "Unable to open image '" << path << "': "
                  << strerror(errno) << "\n";
        return 3;
    }

    struct feature *features;
    int num_features;
    num_features = sift_features(image, &features);

    MatchResults results;
    if (exhaustive_match) {
        results = db->exhaustive_match(features, num_features);
    } else {
        results = db->match(features, num_features, max_nn_chks);
    }

    ResultVector sorted_results;
    for (MatchResults::const_iterator i = results.begin();
         i != results.end();
         i++) {
        sorted_results.push_back(i->second);
    }
    std::sort(sorted_results.begin(), sorted_results.end(), result_score_cmp);

    clock_t end = clock();

    for (ResultVector::const_iterator r = sorted_results.begin();
         r != sorted_results.end();
         r++) {
        std::string label = r->label;
        float score = r->score;
        printf("%s: %f %f\n", label.c_str(), score, r->percentage);
    }

//    std::cerr << "clocks: " << end - start << " " << CLOCKS_PER_SEC << "\n";

    return 0;
}

int main( int argc, char** argv )
{
    int max_nn_chks = 200;
    bool exhaustive_match = false;
    const char *progname = argv[0];

    int ch;
    while ((ch = getopt(argc, argv, "e")) != -1) {
        switch (ch) {
        case 'e':
            exhaustive_match = true;
            break;
        default:
            usage(progname);
            return 1;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2) {
        usage(progname);
        return 1;
    }

    ImageDB db;
    if (!db.load(argv[0])) {
        return 2;
    }

//    do_match(&db, argv[1], max_nn_chks, exhaustive_match);
    return do_match(&db, argv[1], max_nn_chks, exhaustive_match);
}
