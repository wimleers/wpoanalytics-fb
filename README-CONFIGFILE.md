# PatternMiner config file

The config file contains a single JSON object.

**Note:** due to limitations of the current JSON parser (`QxtJSON`), it is
impossible to use comments to annotate the various settings. A possible
work-around is simply adding more properties (e.g. "label"), since these are
ignored anyway.




## Concept hierarchy

A general principle that is applied by PatternMiner is that the colon (`:`) is
the hierarchy separator. That doesn't mean the input data must use the colon as
a hierarchy separator as well, but it does mean that if you want hierarchical
data to be treated as hierarchical, then you should specify what your hierarchy
separator is in the attribute configuration.  
Generally, values are addressable as such:

    `<attribute name>:<val>:<sub>:<etc>`

So, examples are:

* `page:/index.php`
* `duration:slow`
* `browser:Windows:XP:Firefox:6`

These can then be matched with wildcards at any point, e.g.:

* `p*.php` will match `page:/index.php`
* `browser:*:Firefox:*` will match items for any version of Firefox on any OS
* `*Fire*` will match items for "Firefox" and "Firefly"

Currently, this fact is not yet being exploited by the mining algorithms, but
it might in the future. It is already leveraged in the GUI, when using the live
autocomplete filter.

In the literature, this is called a "concept hierarchy". It allows reasoning
about related items (since some items share a common parent), and thus allows
for more meaningful association rules. Again, this is yet to be implemented.




## Division

There are three sections (JSON object keys) at the root level:

1. `metadata`
2. `parser`: optionally filter to a subset of the data set
2. `query`: define the query that you want to apply to the data stream
3. `attributes`: define which attributes should be

I.e. the overall structure is:

    {
      "metadata" : {},
      "parser" : {},
      "query" : {},
      "atributes" : {},
    }

Now we'll look at the settings available within each of these sections.




## `metadata`

This section *can* contain the following keys:

* `dataset`: used when the output format is `RFEJSON`, is used directly in the
output of each association rule that is found.
* `subset`: used when the output format is `RFEJSON`, is used directly in the
output of each association rule that is found. Use an asterisk (`*`) when
mining association rules over the entire data set.

I.e. the overall structure is:

    {
      "metadata" : {
        "dataset" : "some data set",
        "subset" : "*",
      },
      "parser" : {},
      "query" : {},
      "atributes" : {},
    }




## `parser`

These settings *cannot* be changed anymore once patterns have been mined and
mining is continued from the state of a previous mining session.
Unless, of course, you don't mind that data that already exists in the state
(specifically, in the `PatternTree`) will not be compliant with your new
settings. They will gradually (as time and thus the data stream passes) be
updated automatically though.

* `categorical item constraints`: See `query`:`patterns`:`item constraints` for
details. When parsing, the categorical items of a sample are parsed first.
If they do not match the categorical item constraints, the sample is dropped.
* `numerical item constraints`: See `query`:`patterns`:`item constraints` for
details. When parsing, the numerical items of a sample are parsed after the
categorical items, because the categorical item constraints may not be matched,
which allows us to not perform the discretization, which is relatively costly.
If the discretized numerical items do not match the numerical item constraints,
the sample is dropped.

I.e. the overall structure is:

    {
      "metadata" : {},
      "parser" : {
        "categorical item constraints" : {},
        "numerical item constraints" : {},
      },
      "query" : {},
      "atributes" : {},
    }




## `query`

This section is divided into two subsections:

1. `patterns`: settings for pattern mining
2. `association rules`: settings for association rule mining

I.e. the overall structure is:

    {
      "metadata" : {},
      "parser" : {},
      "query" : {
        "patterns" : {},
        "association rules" : {},
      },
      "atributes" : {},
    }




### `query`: `patterns`

These settings *cannot* be changed anymore once patterns have been mined and
mining is continued from the state of a previous mining session.
Unless, of course, you don't mind that data that already exists in the state
(specifically, in the `PatternTree`) will not be compliant with your new
settings. They will gradually (as time and thus the data stream passes) be
updated automatically though.

* `minimum support`: a value between 0 and 1, typically between `0.01` and
`0.05`. If `0.01`, that means a pattern has to occur at least 1% of the time to
be considered "interesting". Any patterns occurring less than 1% of the time
are not presented to the user.
* `minimum potential pattern support`: again a value between 0 and 1, typically
about 50—80% of the value set for `minimum support`. It must always be smaller
than or equal to the value for `minimum support`.  
This value determines the minimum support while analyzing the data stream, i.e.
which patterns are remembered, even if they may not meet the minimum support.
It is needed to remember "subfrequent" patterns because they may become
frequent patterns later: the frequency of patterns may evolve over time.
* `item constraints`: an array of item constraints. Two item constraint types
are supported: positive and negative. Items listed within an item constraint
are OR'ed and support wildcards (thus for negative item constraints: "NOR").
All item constraints together are AND'ed. See the documentation for the
`attributes` section for more information about the transformations items go
through: the item constraints apply to the items as they are defined in the
`attributes` section. Note that it may sometimes be interesting to add negative
item constraints here, just to avoid patterns that are occurring 100% of the
time (which typically occurs when filtering to a subset of the data).  
Sample item constraints are:
  * `[{ "type" : "==", "items" : ["category:foo"]}]`: positive item constraint
    that requires a value of `"foo"` for the attribute `"category"` for the
    pattern to be stored.
  * `[{ "type" : "==", "items" : ["category:foo", "category:bar"]}]`: positive
    item constraint that requires a value of `"foo"` *or* `"bar"` for the
    pattern to be stored.
  * `{ "type" : "!=", "items" : ["category:foo"]}`: negative item constraint
    that prevents a value of `"foo"` for the pattern to be stored.
  * `[{ "type" : "!=", "items" : ["category:foo", "category:bar"]}]`: negative
    item constraint that prevents a value of `"foo"` *or* `"bar"` for the
    pattern to be stored.
  * `[{ "type" : "==", "items" : ["category:*"]}, { "type" : "!=", "items" : ["category:foo", "category:bar"]}]`:
    positive *and* negative item constraint, that accept a pattern that
    contains *any* category (hence the wildcard) *except* the categories "foo"
    and "bar"
* `tilted time window definition`: the definition of the tilted time window
that should be used. `"900:QQQQH"` will create a new time window of the smallest
(most granular) granularity every 900 seconds (15 minutes, or a **Q**uarter),
and after 4 quarters have been analyzed, their respective supports will be
summed and stored in the next granularity (the **H**our granularity).  
Use `""` for the default tilted time window definition, which is
`"3600:HHHHHHHHHHHHHHHHHHHHHHHHDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"`. This contains
24 windows of one hour and 30 windows of one day. That means up to a month of
data is stored with a per-day granularity and the data for the current day is
stored with a per-hour granularity.  
There are no limits to the number of windows or number of granularities. One
unsigned int (32 bits) of memory is used per window, per pattern.

I.e. the overall structure is:

    {
      "metadata" : {},
      "parser" : {},
      "query" : {
        "patterns" : {
          "minimum support" : 0.05,
          "minimum potential pattern support" : 0.03,
          "item constraints" : [
            { "type" : "==", "items" : ["category:*"]},
            { "type" : "!=", "items" : ["category:foo", "category:bar"]},
          ],
          "tilted time window definition" : "",
        },
        "association rules" : {},
      },
      "atributes" : {},
    }




### `query`: `association rules`

These settings *can* be changed whenever you want to mine association rules
from a state file that you've loaded. In fact, you *should* change these to find
different rules that may all be interesting to you.

* `minimum confidence`: a value between 0 and 1. The confidence of a rule
indicates how often the consequent occurs given the antecedent has occurred.
For example, {"country:Belgium"} => {"weather:rain"} probably has a confidence
of about 70%, which means that it rains about 70% of the time in Belgium.
* `antecedent item constraints`: item constraints that the antecedent of the
association rule must match. See `query`:`patterns`:`item constraints` for
details.
* `consequent item constraints`: analogously to `antecedent item constraints`.

I.e. the overall structure is:

    {
      "metadata" : {},
      "parser" : {},
      "query" : {
        "patterns" : {},
        "association rules" : {
          "minimum confidence" : 0.10,
          "antecedent item constraints" : [
            { "type" : "==", "items" : ["event:*"] },
          ],
          "consequent item constraints" : [
            { "type" : "==", "items" : ["circumstance:*"] },
          ],
        },
      },
      "atributes" : {},
    }




## `attributes`

The attributes *cannot* be changed anymore once patterns have been mined and
mining is continued from the state of a previous mining session.
Unless, of course, you don't mind that data that already exists in the state
(specifically, in the `PatternTree`) will not be compliant with your new
attribute configuration. They will gradually (as time and thus the data stream
passes) be updated automatically though.

**Note:** as explained in the README, this is what a sample looks like:

    { "int" : {"time":1316383200, "some value":42}, "normal" : {"category":"foo", "section":"bar"}, "denorm" : {"ip address":"127.0.0.1"}}

This section is divided into two subsections:

1. `categorical`: categorical attributes (nominal & ordinal). These are values
   of attributes in the `"normal"` and `"denorm"` sections of a sample.
2. `numerical`: numerical attributes (interval & ratio). These are values of
   attributes in the `"int"` section of a sample.


I.e. the overall structure is:

    {
      "metadata" : {},
      "parser" : {},
      "query" : {},
      "atributes" : {
        "categorical" : {},
        "numerical" : {},
      },
    }


For each attribute, the name of the attribute in the sample is used to
correlate them. Thus, to define settings for the attribute `"some value"`, we
must also create a key `"some value"` inside the `"numerical"` object.

**Note:** if you wish to use an attribute, but don't need to set any settings,
you still have to list the attribute, albeit you can assign it an empty object!

The following settings can be defined for each attribute:

* `name`: to rename an attribute; to make something cryptic less cryptic. For
  example: `"gen_time" : { "name" : "server_time" }` would rename the
  `gen_time` attribute to `serverTime`. Note that this means you must also use
  `serverTime` in item constraints, since this is how it will be identified
  after being parsed!


The following settings can be defined for `"normal"`s:

* `parentAttribute`: define which attribute is a parent and thus whose value
  should be prepended to the current value, and separated with the hierarchy
  separator. E.g. the attribute containing the browser version should be
  prepended with the attribute containing the browser name, to generate items
  such as "browser:Firefox:6" and "browser:Chrome:7" instead of just the
  meaningless "browserVersion:6" and "browserVersion:7".  
  (Also see the "Concept hierarchy" section in this README.)
* `hierarchySeparator`: set the hierarchy separator if the value of an
  attribute contains hierarchical data. The value will be split on the string
  you define for this setting and a separate item will be created for each of
  the levels of the hierarchy (i.e. first level only, first two levels …).


The following settings can be defined for `"int"`s:

* `discretizations`: as stated in the README, the current implementation only
  supports discrete values. That means it cannot deal with integer numbers, let
  alone floating point numbers. Hence we must define discretizations for them.
  Multiple discretizations can be defined for one numerical attribute: a
  default discretization and as many conditional discretizations as needed.
  Sample discretizations are:
  * `[{"thresholds" : { "fast":1000, "ok":1200, "slow":-1}}]`: only a default
    discretization is specified, with anything up to a thousand (ms) being
    marked as `"slow"`, anything above 1000 but up to 1200 being marked as
    `"ok"` and anything above 1200 being marked as `"slow"`.
  * <pre>
        [
          {
            "thresholds" : { "fast":1200, "ok":2200, "slow":-1 },
            "circumstances" : ["connection:broadband"],
          },
          {
            "thresholds" : { "fast":2000, "ok":3500, "slow":-1 },
            "circumstances" : ["connection:mobile"],
          },
          {
            "thresholds" : { "fast":1700, "ok":2800, "slow":-1 },
            "circumstances" : ["connection:mobile", "country:BE"],
          },
          {
            "thresholds" : { "fast":1500, "ok":2400, "slow":-1 },
          },
        ]
    </pre>  
  This may indeed be a lengthy example, but it is definitely not convoluted.
  The default is at the bottom. The top one lists more stringent requirements
  for broadband connections. The second one lists less stringent requirements
  for mobile connections. And the third one lists slightly more stringent
  requirements for mobile connections in Belgium than mobile connections in
  general.
* `isEpisode`: boolean; whether this measurement should be considered an
  independent event: an "episode". E.g. when measuring multiple stages of the
  page loading process, you want to find association rules about an episode in
  relation to its circumstances (all other attributes except for others marked
  as episodes), not related to other episodes. It is still possible to find
  association rules between episodes though: simply remove or disable the
  `"isEpisode"` settings.  
  When enabled for e.g. a `"transferTime"` attribute with a discretized value
  of `"slow"` will not be stored as a single `"transferTime:slow"` item (as it
  would when this setting is disabled), but instead *two* items will be
  generated: `"episode:transferTime"` and `"duration:slow"`. "episode" and
  "duration" are currently hardcoded, since they're the common use case.


I.e. the overall structure is:

    {
      "metadata" : {},
      "parser" : {},
      "query" : {},
      "atributes" : {
        "categorical" : {
          "browser" : {},
          "browserVersion" : {
            "name" : "browser",
            "parentAttribute" : "browser",
          },
          "country" : {},
          "connection_speed" : {},
          "page" : {
            "hierarchySeparator" : ":",
          },
        },
        "numerical" : {
          "network" : {
            "name" : "networkTime",
            "isEpisode" : true,
            "discretizations" : [
              {
                "thresholds" : { "fast":600, "ok":1200, "slow":-1 },
              },
            ]
          },
          "e2e" : {
            "name" : "endToEndTime",
            "isEpisode" : true,
            "discretizations" : [
              {
                "thresholds" : { "fast":1200, "ok":2200, "slow":-1 },
                "circumstances" : ["connection:broadband"],
              },
              {
                "thresholds" : { "fast":2000, "ok":3500, "slow":-1 },
                "circumstances" : ["connection:mobile"],
              },
              {
                "thresholds" : { "fast":1700, "ok":2800, "slow":-1 },
                "circumstances" : ["connection:mobile", "country:BE"],
              },
              {
                "thresholds" : { "fast":1500, "ok":2400, "slow":-1 },
              },
            ]
          },
        },
      },
    }

That is all.
