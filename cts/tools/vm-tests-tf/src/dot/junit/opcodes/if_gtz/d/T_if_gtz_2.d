.source T_if_gtz_2.java
.class public dot.junit.opcodes.if_gtz.d.T_if_gtz_2
.super java/lang/Object


.method public <init>()V
.limit regs 1

       invoke-direct {v0}, java/lang/Object/<init>()V
       return-void
.end method

.method public run(F)I
.limit regs 6

       if-gtz v5, Label9
       const/16 v5, 1234
       return v5

Label9:
       const/4 v5, 1
       return v5
.end method
