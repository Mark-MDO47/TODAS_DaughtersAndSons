count=1
for v in `ls e*.wav`
do
  git mv $v `echo $v | sed "s?^[0-9][0-9][0-9][0-9][_]*??" | sed "s?^?$(printf "%04d_" ${count})?"`
  ((count++))
done

for v in `ls o*.wav`
do
  git mv $v `echo $v | sed "s?^[0-9][0-9][0-9][0-9][_]*??" | sed "s?^?$(printf "%04d_" ${count})?"`
  ((count++))
done

for v in `ls B*.wav`
do
  echo mv $v `echo $v | sed "s?^[0-9][0-9][0-9][0-9][_]*??" | sed "s?^?$(printf "%04d_" ${count})?"`
  ((count++))
done
