# hashpan

## TL;DR

This is a highly optimized implementation, written for OpenCL in C (with a
few snazzy tricks up the sleeves to make the amount of work more attainable.)

This thing computes an entire IIN in under 1.5 minutes (under two hours for
the whole shabam) on my oldish Macbook Air (but I'd love to have a shot at
running it on a Mac Pro)

* OpenCL the whole way through
* UTHash (with a custom hashset built on top of it)
* SHA1 reference implementation built inside of a custom OpenCL Kernel
* passing around minimal data (checkbits only)
* woooo

This was my first experience with OpenCL too - good stuff

## A little bit more detail

## The whole story

I've always (okay, not always, but for a while) known that there was some
format to a credit card number other than just random digits.  I just never
really knew what it was.

A little Wikipedia action showed me that the first six digits on a 16 digit card
were the "IIN" (a number identifying the issuing bank).  The last digit was a
checkbit to prevent transmission errors (for some reason I had always
suspected this bit was so that gas stations in the middle of nowhere with no
internet connection could rule out obvious fraud but apparently that's not the
case).  Luhn algorithm.

So I looked at `pans.txt` and noticed a lot of repeated first six digits.  I
took that as a hint that I should probably start with those IINs and see where
it led me.

So I wrote up a simple Ruby script, that just ran through all the numbers,
determined if they passed a luhn check (to cut down on the number of SHAs I
had to generate) and compared them to a HashMap of the digests from
base64 decoding the supplied hashes.

Slooowwwwww

So I rewrote that thing in Java.  Definitely faster, but this isn't
an single-order-of-magnitude-makes-a-difference problem.  I thought it would
be cool to play around with doing it on OpenCL - which I had heard about but
never had a chance to play with.  I found a few libraries that wrap OpenCL,
most notably aparapi (https://code.google.com/p/aparapi/).  aparapi doesn't
even require that you write your kernels in C - which is badass.

Getting the luhn check to work in aparapi was pretty much a breeze (I even
was happy to make the luhn check operate directly on longs - check out my luhn
implementation - I'm pretty excited about it), but when I went
to implement the SHA1 on aparapi, I ran into all kinds of issues.  This is
because aparapi doesn't support Java constructs like catch exceptions, or using
new (which is pretty much a must-have for any OTB Java SHA1 implementation).

I decided if I was going to go write the kernel in C, I might as well write
the whole thing in C - so I started over again..

I wrote what I had already in C, and then I took the SHA1 C reference
implementation and after some fuss, successfully wrapped a kernel around it.
Now I was pumping out hashes like a monster.

Now the next piece of the puzzle was the lookups.  C doesn't have a HashSet or
Map you can just do anything you please with whenever you want, so I ended up
pulling in UTHash (which I've also never used before) and writing a custom
Set implementation (johnset lol) on top of it.  I used a hash function to convert
the char arrays into `uint` so I could pass around less data, and while I was
at it, I actually computed the hashtable values in OpenCL as part of the hashing
operation.  Everything happens in OpenCL except the final check to determine if
a hash is contained in the result set.

[intermission]

So now for each IIN I was computing 10bn combinations.  That's a lot, so I
started thinking it'd be neat if I could instantly jump from one valid number to
the next.  I wasted a bunch of time on that, before I came up with the idea that
instead of running over every card number, I could run over all but the last
bit, and compute the appropriate checkbit.  Booyah, no more shrinking arrays to
get rid of the non-luhn.  It was alllll luhn now - and 1/10 the work.

Then I spent a bunch of time tweaking and pinching bits (which is a big deal
in OpenCL) - most notably the luhn check actually only sends back checkbits -
with arrays offsets for the CC numbers that they correspond to (check it out!).

Then, I was ready - and that's right now - so I'm going to go clean up some
rough edges, and submit a PR.

## Where Next

Some performance improvements I almost had working, but ended up pulling:

* I played around with, but ultimately "ripped out" the SHA1 implementation
  from JohnTheRipper

* Synchronizing work on CPU and GPU to run concurrenly.  Already my laptop's
  screen flickers and basically stops working - but who knows, maybe I could
  push it further

## In case

iins.txt obtained like this:

``` ruby
File.write('./data/iins.txt', File.readlines('./data/pans.txt').map { |l| l[0..5] }.uniq.join("\n"))
```

just didn't feel like muddling my C code with it

## Thanks

Thanks so much for the challenge :)  I had a good time learning OpenCL, getting
back into C, crashing my laptop more times than I care to say, and spending
the last week thinking about how to make this faster.

Looking forward to the next challenge!
