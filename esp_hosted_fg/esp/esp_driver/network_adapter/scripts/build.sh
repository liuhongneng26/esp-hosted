#!/bin/bash

# . ./scripts/env.sh

./scripts/build_app.sh || exit -1

./scripts/flash_sample.sh