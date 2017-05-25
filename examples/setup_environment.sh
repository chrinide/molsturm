# The relative path to the build path
BUILD_PATH="../build"

# ------------------------------------------------

for dir in $PWD/../src/molsturm_iface $PWD/$BUILD_PATH/src/molsturm_iface; do
	dir=$(realpath $dir)
	if [ ! -d "$dir" ]; then
		echo "Not a valid path:  $dir" >&2
		echo "Probably the examples won't work" >&2
	fi
	export PYTHONPATH="$PYTHONPATH:$dir"
done

# ------------------------------------------------

for file in $PWD/../../adcc/build/src/pyadc/pyadc_iface.py $PWD/../../adcc/src/pyadc/pyadc.py; do
	if [ -f "$file" ]; then
		dir=$(realpath $(dirname $file))
		export PYTHONPATH="$PYTHONPATH:$dir"
	else
		break
	fi
done

python3 <<- EOF
from molsturm.posthf import available_methods
print("Available post-HF methods:")
for m in available_methods:
  print("  ",m)
EOF
