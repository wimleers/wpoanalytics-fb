# PatternMiner

## Introduction

PatternMiner is an application for performing association rule mining on data
streams. More accurately, it performs frequent pattern mining ("Which sets of
frequent items are occurring together frequently?"), storing the frequencies
in a "tiled time window". This means that the most recent data is stored in
full detail; the oldest data is stored with the least granularity. This helps
keep memory requirements low. Irrelevant data is automatically purged. All
data is stored in a trie structure called the "PatternTree". This PatternTree
can be used to mine association rules *very* efficiently. One can mine rules
over any time range that is contained in the TiltedTimeWindow and one can set
"item constraints" that have to be matched for a rule to be output. Hence one
can look for very specific association rules.

Note that in the current implementation, only *discrete* attributes are
supported. Hence continuous data (such as durations, temperature …) need to be
discretized. The implementation supports discretizations, even conditional
discretizations.  
(An "item" is an *instance* of an "attribute". e.g. "Firefox" is a "browser".)

The flow is:

    1 log line → 1 sample → 1…n transactions
      1 transaction is a set of m items; m may vary for every sample
      an "item" is an instance of an "attribute" (see above)
      "itemset" is a synonym for "transaction"
    1 transaction → 0…1 itemset containing only frequent items → store in PatternTree
      (note that "almost frequent" itemsets are also put in the PatternTree)
      "itemset containing only frequent items" = "frequent itemset"
      "frequent itemset" is a synonym for "pattern"
      (patterns that become infrequent as time passes are purged)
    extract parts of PatternTree → o frequent patterns → p association rules


Association rules are of the form

    {antecedent} ⇒ {consequent} with:
      - X% confidence
      - Y support
      - Z% relative support

Explanation:

* `{antecedent}` and `{consequent}` are sets of items.
* "confidence" is a percentage that indicates that given the antecedent, how
  likely it is that the consequent will occur.
* "support" is the data mining term for "frequency". That means this particular
  association turned out to be true Y times. Or: the union of the antecedent
  and the consequent (which together form a frequent pattern) occurred Y times.
* "relative support" is Y divided by the number of samples over which this was
  calculated. It gives you a feel for how often this association occurs over
  the entire data set. Compare to "confidence", that says how often the rule
  occurs when you only look at transactions that contain the antecedent.




## History

Originally called WPO Analytics, and based on [Wim Leers](http://wimleers.com)'s
[master thesis](http://wimleers.com/tags/master-thesis). For his master thesis,
it was designed to analyze [Episodes](http://stevesouders.com/episodes/) log
files (Episodes is a JS framework for instrumenting the various parts of the
page loading process). While interning at Facebook, Wim abstracted this to be
more generally useful.




## License

The original code (from the master thesis) was in the public domain, using the
UNLICENSE copyright waiver. Since Facebook requires code to be under the Apache
License for it to be contributed upstream, that's what has happened.

See the `LICENSE` file for full details.




## Structure

The goal is to have each major piece of functionality as a "module" (or
whatever you call it), and thus also in a separate directory. All code in that
directory must be wrapped in a namespace that's named identically to the
directory name.

(This is what already allowed the original parser to be easily replaced by a
new parser, for example.)

The major modules are: `Config`, `Parser`, `Analytics`, `CLI` and `UI`.

Initially, each module was strictly independent and connected with Qt's
[signal/slot mechanism](http://doc.qt.nokia.com/stable/signalsandslots.html).
However, that required each of the data types sent through those signals to be
known on both sides. So there was mostly a resort to generic data types, which
caused large parameter quantities and inefficiencies. Hence the `common` module
was introduced.

    patternminer/

        Analytics/
          "Analytics" functionality: pattern + rule mining.
          Thus includes implementations of the FP-Growth, FP-Stream and Apriori
          algorithms.

        CLI/
          Command-line interface to use all functionality

        common/
          Low-level data types shared by multiple modules.

        Config/
          In-memory representation of the JSON config file + parser for it.

        Parser/
          Default parser: supports "JSON logs": every line is a JSON object.

        shared/
          Classes that can be used by multiple modules, e.g. a JSON parser.

        TailerSwiftParser/
           A module that inherits from Parser; uses TailerSwift's `swift_event`
           events to delineate boundaries between time windows; hence no time
           needs to be spent evaluating timestamps.
           (THIS IS THE ONLY FACEBOOK-SPECIFIC CODE)

        UI/
          Graphical interface to use all functionality.


There is only one external dependency: Qt.


## Data stream format (input data)

The data stream format is very simple. Each sample in the data stream:

* is terminated by a newline character (`\n`)
* is a JSON object that contains up to 3 other JSON objects:
  * `int`: all numerical attributes
  * `normal`: all normalized attributes (i.e. limited range of values)
  * `denorm`: all denormalized attributes (i.e. *unlimited* range of values)

Example of a valid sample:

    { "int" : {"time":1316383200, "some value":42}, "normal" : {"category":"foo", "section":"bar"}, "denorm" : {"ip address":"127.0.0.1"}}


## Code style

This project follows the [Qt coding style](http://wiki.qt-project.org/Coding_Style)
and [Qt coding conventions](http://wiki.qt-project.org/Coding_Conventions).




## Tips

Whenever possible, [`QDebug` output operators](http://doc.qt.nokia.com/latest/qdebug.html#writing-custom-types-to-a-stream)
are used to simplify debugging. When you're debugging the application, you can
`qDebug() << something` with almost *any* piece of data. Often, PatternMiner 
internally works with data structures that contain *a lot* of data, in several
cases linked together by pointers. That's near impossible to step through
manually. Hence these operators are defined. For example, `qDebug() << fptree;`
will result in a nice tree structure being printed.  
By using `#ifdef DEBUG` checks, these operator definitions are compiled away.  
When building in release mode, `qDebug()` calls are also automatically compiled
away, so you don't need to remove those manually or wrap them in `#ifdef DEBUG`
checks.




## Building

### Using `fbconfig`/`fbmake`

These commands must be run at the root of your `fbcode` checkout.

#### Debug mode

    fbconfig patternminer && fbmake allclean && fbmake dbg && ./_bin/patternminer/patternminer

#### Release mode

    fbconfig patternminer && fbmake allclean && fbmake opt && ./_bin/patternminer/patternminer




### Qt's build system

Qt's build system is designed to be portable. It is basically a (fairly nice,
but still not nice enough) abstraction for `Makfile`s that is also capable of
generating Visual Studio projects (`.vcproj`) and Xcode projects
(`.xcodeproj`).

Because this application also comes with a GUI, this is of particular
importance, because it means that you can actually use the GUI on any platform
(Mac OS X, Windows, GNU/Linux or even more exotic cases if you'd like, such as
Meego, iOS or embedded Linux).




### Using Qt Creator

Install Qt Creator. Then open any of the .pro files; Qt Creator will open
automatically. Click the green arrow at the bottom left and off you go.

**Note:** I recommend configuring Qt Creator to set the `-j 8` argument for
`make`. To do this, go to Projects → Build settings → Build steps → Make →
Details → Make arguments and enter `-j 8`. Your builds will now be
*significantly* faster!




### Using `qmake`/`make`

The following commands can simply be copy/pasted and executed. They build it with 8 concurrent jobs and then execute the binary.


#### CLI in debug mode

    qmake patternminer-CLI.pro && make -j 8 && ./patternminer


#### CLI in release mode

    qmake patternminer-CLI.pro -config release && make -j 8 && ./patternminer


#### GUI in debug mode

    qmake patternminer-GUI.pro && make -j 8 && open "PatternMiner GUI.app"


#### GUI in release mode

    qmake patternminer-GUI.pro -config release && make -j 8 && open "PatternMiner GUI.app"


#### CLI for Facebook servers in debug mode

    /home/engshare/third-party/gcc-4.6.2-glibc-2.13/qt/qt-4.7.4/bin/qmake patternminer-CLI-Facebook-servers.pro -config debug && make -j 8 && ./patternminer


#### CLI for Facebook servers in release mode

    /home/engshare/third-party/gcc-4.6.2-glibc-2.13/qt/qt-4.7.4/bin/qmake -config release patternminer-CLI-Facebook-servers.pro -config release && make -j 8 && ./patternminer
