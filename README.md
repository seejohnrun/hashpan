

I've known for a long time that there was a checkbit in credit card numbers,
but I never really looked into how they worked until now.

My initial implementation (in Ruby) was a basic Luhn-check followed by a SHA1
and comparison against a map of stored post-b64 from hashes.txt

Obviously, this one was pretty slow and wouldn't be the answer.

I rewrote that jam in Java - faster, but not by much especially for large runs.

Then I got the idea to perform the computations using OpenCL.  I tried first
implementing that in Java on top of aparapi.  Aparapi was a great experience,
and definitely a project I'd come back at for another look - but not
something that was going to give me the flexibility I needed to be able
to experiment (no exceptions, no new operator, means that using basically
any Java algorithm implementation as part of a Kernel is a non-starter)

So I went over to C and started writing.

I needed a fast SHA1 algorithm, one which I ended up "ripping" out of John
the Ripper (the popular password cracking tool) and making a few modifications
to in order to fit my needs.

Now I had the luhn check (a fast implementation I wrote myself after a few
iterations of playing with other implementations), and the SHA1 running on
top of OpenCL.  The SHA1 was running on the GPUs and the Luhn on the CPUs.

I thought of a way to cut the work the luhn-check would have to do down, by
abusing the fact that the checkbit means that once I find a value, nothing
else with those starting digits will be valid.
