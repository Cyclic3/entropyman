# entropyman
_sometimes optimal hangman engine_

## Theory
Ideally, we would like to take the path with the least entropy at each point.
Unfortunately, the game tree is rather large, and so this becomes infeasible
even at 2 letters.

As such, we make an estimation of the entropy at each point, by recursively
calculating probability-weighted best-path entropy, all the way down to some
depth (which may not be uniform), and assuming the entropy is `log2(n)` at
each point. The engine could potentially be made more effective with a more
realistic choice of probability distribution than people randomly selecting
words from a list, but this seems to give reasonably good results.

However, this approach runs into three issues:

### Unsolvability
We will examine nodes at which there are no candidates left in the system.
With the standard hyperreal extension `log2(0) = -infty`, we will end up with
an engine that will perform fine, until it sees a way to lose, at which point
it will immediately try to lose to the exclusion of all other goals.

The naive approach is to use `log2(0) = infty`, so that the engine percieves
losing as the same as some unsolvable problem (as indeed it is). However, this
ends up with the exact opposite problem: if the AI is backed into a corner 
where it can see that there is a definite non-zero possiblity of losing, it
will see all states as equivalently bad, and be unable to make a decision

The solution I have found is to use a reasonably large number as the entropy
result for failure. This number needs to be large enough to disuade the
engine, but not so large that we hit precision issues. I have found both
1000 and 2^32 to be sufficient values, but tweaking this may allow a more soft
algorithm that is willing to take chances.

### Final step
Once we hit upon an optimal game tree, we know exactly what to expect at each
point, and so our entropy drops to zero. This is fine, as at this point all
nodes evaluate to either 0 or `log2(0)`, constaining the algorithm to stick to
the right path.

However, we need to make sure that we do not expand the victory nodes, as they
have no descendents, and therefore would be transformed into parents of failure
nodes

### Equivalent paths
Once an optimal game tree is found, the engine has nothing to work towards
except not losing, which should be inevitable. However, in the mean time, it
will often start trolling by wasting turns (making guesses that cannot do
anything but lose a life).

It would be possible to patch out this behaviour, but it's funny.
