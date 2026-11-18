#!/usr/bin/env python3
import argparse
import itertools
import os
import random
from multiprocessing import Pool

TOPICS = {
    "space": "space exploration rocket orbit planet mars moon satellite "
    "astronaut telescope galaxy star launch spacecraft gravity mission "
    "asteroid comet nebula cosmos universe",
    "medicine": "medicine patient doctor hospital disease treatment vaccine "
    "virus surgery diagnosis therapy symptom clinical drug infection "
    "immune health nurse cancer heart",
    "finance": "finance market stock investor bank interest inflation credit "
    "loan currency economy trading profit revenue budget tax capital "
    "dividend portfolio debt",
    "sport": "sport football team player coach match season league goal "
    "tournament championship score stadium athlete training victory "
    "defeat referee olympic medal",
    "music": "music song album band concert guitar piano singer melody rhythm "
    "orchestra composer lyrics jazz rock audience stage tour record "
    "violin",
    "cooking": "cooking recipe kitchen chef flour sugar butter oven bake "
    "dinner sauce vegetable spice garlic onion pasta bread soup salad "
    "dessert",
    "climate": "climate weather temperature carbon emission warming ocean "
    "glacier drought storm rainfall forest energy solar wind pollution "
    "ecosystem species flood ice",
    "computing": "computer software program code algorithm processor memory "
    "network server database thread parallel compiler linux internet "
    "security data cloud cache kernel",
    "history": "history empire king war battle ancient century revolution "
    "dynasty kingdom castle army treaty colony monarch medieval "
    "civilization ruins archive chronicle",
    "travel": "travel journey city hotel flight airport passport beach "
    "mountain museum tourist island village train map luggage guide "
    "adventure border culture",
}

COMMON = (
    "time year people way day man thing woman life child world school state "
    "family student group country problem hand part place case week company "
    "system question work government number night point home water room "
    "mother area money story fact month lot right study book eye job word "
    "business issue side kind head house service friend father power hour "
    "game line end member law car city community name president team minute "
    "idea kid body information back parent face others level office door "
    "health person art war history party result change morning reason "
    "research girl guy moment air teacher force education new good first "
    "last long great little own other old big high different small large "
    "next early young important few public bad same able make take know "
    "think see come want look use find give tell call try ask need feel "
    "become leave put mean keep let begin seem help show hear play run move "
    "live believe bring happen write provide sit stand lose pay meet include "
    "continue set learn lead understand watch follow stop create speak read "
    "spend grow open walk win offer remember love consider appear buy wait "
    "serve die send expect build stay fall cut reach kill remain suggest "
    "raise pass sell require report decide pull"
).split()

SYLLABLES = (
    "ba be bi bo bu da de di do du ka ke ki ko ku la le li lo lu ma me mi mo "
    "mu na ne ni no nu ra re ri ro ru sa se si so su ta te ti to tu va ve vi "
    "vo vu za ze zi zo zu ar er ir or ur an en in on un"
).split()

STOP_WORDS = "the of and to in is it that was for on with as at by from".split()

TAIL_WORDS = 30_000
TOPIC_SHARE = 0.25
STOP_SHARE = 0.35
WORDS_PER_LINE = 14
AVERAGE_WORD_BYTES = 6


def build_tail(seed):
    rng = random.Random(seed)
    words = set()
    while len(words) < TAIL_WORDS:
        words.add("".join(rng.choices(SYLLABLES, k=rng.randint(2, 4))))
    return sorted(words)


def zipf_weights(count):
    return list(itertools.accumulate(1.0 / (rank + 1) for rank in range(count)))


def write_document(args):
    index, out, size, seed, tail = args
    rng = random.Random(seed * 1_000_003 + index)
    topic = rng.choice(sorted(TOPICS))
    topic_words = TOPICS[topic].split()
    general = COMMON + tail
    general_weights = zipf_weights(len(general))

    word_count = max(50, size // AVERAGE_WORD_BYTES)
    topic_count = int(word_count * TOPIC_SHARE)
    stop_count = int(word_count * STOP_SHARE)
    words = (
        rng.choices(topic_words, k=topic_count)
        + rng.choices(STOP_WORDS, k=stop_count)
        + rng.choices(general, cum_weights=general_weights,
                      k=word_count - topic_count - stop_count)
    )
    rng.shuffle(words)

    lines = [f"{topic.title()} report {index}"]
    for start in range(0, len(words), WORDS_PER_LINE):
        line = " ".join(words[start:start + WORDS_PER_LINE])
        lines.append(line[0].upper() + line[1:] + ".")

    directory = os.path.join(out, topic)
    os.makedirs(directory, exist_ok=True)
    path = os.path.join(directory, f"doc_{index:05d}.txt")
    with open(path, "w", encoding="ascii") as file:
        file.write("\n".join(lines) + "\n")
    return os.path.getsize(path)


def main():
    parser = argparse.ArgumentParser(
        description="Generates a temporary synthetic English dataset.")
    parser.add_argument("--out", default="data/generated")
    parser.add_argument("--files", type=int, default=10_500)
    parser.add_argument("--size-gb", type=float, default=2.1)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--workers", type=int, default=os.cpu_count())
    args = parser.parse_args()

    average = int(args.size_gb * 1024**3 / args.files)
    rng = random.Random(args.seed)
    sizes = [int(average * rng.uniform(0.5, 1.5)) for _ in range(args.files)]
    tail = build_tail(args.seed)
    tasks = [(i, args.out, sizes[i], args.seed, tail) for i in range(args.files)]

    total = 0
    with Pool(args.workers) as pool:
        for done, size in enumerate(pool.imap_unordered(write_document, tasks,
                                                        chunksize=16), 1):
            total += size
            if done % 1000 == 0 or done == args.files:
                print(f"{done}/{args.files} files, {total / 1024**3:.2f} GB")


if __name__ == "__main__":
    main()
