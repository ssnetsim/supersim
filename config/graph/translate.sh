#!/bin/bash

set -e

for ext in ".pb" ".textproto"; do
    bazel run @com_google_paragraph//paragraph/translation:graph_translator -- \
	--input_graph ${PWD}/graph.textproto \
	--translation_config ${PWD}/translation.json \
	--output_dir ${PWD}/ \
	--output_ext ${ext}
done


