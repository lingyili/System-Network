The microcode compiler can be used as below:
"java -jar MICOCompiler.jar -in <filename>.xml"

This microcompiler supports additional states such as

FETCH01
FETCH02
FETCH03
FETCH04
FETCH05

EI0
EI1

DI0
DI1

RETI0
RETI1
RETI2

and some additional BONUS states (shown in the file) that you will need for
implementing the interrupt handler.
