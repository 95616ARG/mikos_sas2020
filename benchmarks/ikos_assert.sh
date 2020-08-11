#!/bin/bash

find SVC -type f -print0 | xargs -0 sed -i 's/extern\s*void\s*__VERIFIER_error[^;]\+;/extern void __ikos_assert(int condition);/g'

find SVC -type f -print0 | xargs -0 sed -i 's/__VERIFIER_error(\s*)/__ikos_assert(0)/g'

find SVC -type f -print0 | xargs -0 sed -i 's/__VERIFIER/__ikos/g'
