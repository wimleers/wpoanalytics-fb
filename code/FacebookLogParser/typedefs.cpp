#include "typedefs.h"


namespace FacebookLogParser {

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const Episode & e) {
        dbg.nospace() << e.IDNameHash->value(e.id).toStdString().c_str()
                      << "("
                      << e.id
                      << ") = "
                      << e.duration;

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const EpisodeList & el) {
        QString episodeOutput;

        dbg.nospace() << "[size=" << el.size() << "] ";
        dbg.nospace() << "{";

        for (int i = 0; i < el.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";

            // Generate output for episode.
            episodeOutput.clear();
            QDebug(&episodeOutput) << el[i];

            dbg.nospace() << episodeOutput.toStdString().c_str();
        }
        dbg.nospace() << "}";

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const CircumstanceList & cl) {
        dbg.nospace() << "[size=" << cl.size() << "] ";
        dbg.nospace() << "{";

        for (int i = 0; i < cl.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";

            dbg.nospace() << cl.at(i).toStdString().c_str();
        }
        dbg.nospace() << "}";

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const Sample & sample) {
        const static char * eol = ", \n";
        dbg.nospace() << "{\n"
                      << "time = " << sample.time << eol
                      << "episodes = " << sample.episodes << eol
                      << "circumstances = " << sample.circumstances << eol
                      << "}";

        return dbg.nospace();
    }
#endif
}