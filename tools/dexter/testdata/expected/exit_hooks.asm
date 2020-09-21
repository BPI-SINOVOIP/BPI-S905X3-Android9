
method Target.<init>():void
{
	.src "exitHooks.java"
	.line 28
	.prologue_end
	.line 28
	    0| move-object v0, v2
	.local v0, "this", Target
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Object.<init>():void
	    5| return-void
}

method Target.main(java.lang.String[]):void
{
	.params "?"
	.src "exitHooks.java"
	.line 32
	.prologue_end
	.line 32
	    0| move-object v0, v3
	.local v0, "args", java.lang.String[]
	    1| sget-object v1, java.lang.System.out
	    3| const-string v2, "Hello, world!\n{"
	    5| invoke-virtual {v1,v2}, java.io.PrintStream.println(java.lang.String):void
	.line 33
	    8| invoke-static {}, Target.test():void
	.line 34
	   11| sget-object v1, java.lang.System.out
	   13| const-string v2, "}\nGood bye!"
	   15| invoke-virtual {v1,v2}, java.io.PrintStream.println(java.lang.String):void
	.line 35
	   18| return-void
}

method Target.test():void
{
	.src "exitHooks.java"
	.line 39
	.prologue_end
	.line 39
	    0| new-instance v1, Target
	    2| move-object v8, v1
	    3| move-object v1, v8
	    4| move-object v2, v8
	    5| invoke-direct {v2}, Target.<init>():void
	    8| move-object v0, v1
	.line 40
	.local v0, "obj", Target
	    9| sget-object v1, java.lang.System.out
	   11| const-string v2, "Object(true)   : %s\n"
	   13| const/4 v3, #+1 (0x00000001 | 1.40130e-45)
	   14| new-array v3, v3, java.lang.Object[]
	   16| move-object v8, v3
	   17| move-object v3, v8
	   18| move-object v4, v8
	   19| const/4 v5, #+0 (0x00000000 | 0.00000)
	   20| move-object v6, v0
	   21| const/4 v7, #+1 (0x00000001 | 1.40130e-45)
	   22| invoke-virtual {v6,v7}, Target.testObject(boolean):java.lang.Object
	   25| move-result-object v6
	   26| aput-object v6, v4, v5
	   28| invoke-virtual {v1,v2,v3}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   31| move-result-object v1
	.line 41
	   32| sget-object v1, java.lang.System.out
	   34| const-string v2, "Object(false)  : %s\n"
	   36| const/4 v3, #+1 (0x00000001 | 1.40130e-45)
	   37| new-array v3, v3, java.lang.Object[]
	   39| move-object v8, v3
	   40| move-object v3, v8
	   41| move-object v4, v8
	   42| const/4 v5, #+0 (0x00000000 | 0.00000)
	   43| move-object v6, v0
	   44| const/4 v7, #+0 (0x00000000 | 0.00000)
	   45| invoke-virtual {v6,v7}, Target.testObject(boolean):java.lang.Object
	   48| move-result-object v6
	   49| aput-object v6, v4, v5
	   51| invoke-virtual {v1,v2,v3}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   54| move-result-object v1
	.line 42
	   55| sget-object v1, java.lang.System.out
	   57| const-string v2, "double         : %s\n"
	   59| const/4 v3, #+1 (0x00000001 | 1.40130e-45)
	   60| new-array v3, v3, java.lang.Object[]
	   62| move-object v8, v3
	   63| move-object v3, v8
	   64| move-object v4, v8
	   65| const/4 v5, #+0 (0x00000000 | 0.00000)
	   66| move-object v6, v0
	   67| const/4 v7, #+3 (0x00000003 | 4.20390e-45)
	   68| invoke-virtual {v6,v7}, Target.testDouble(int):double
	   71| move-result-wide v6:v7
	   72| invoke-static {v6,v7}, java.lang.Double.valueOf(double):java.lang.Double
	   75| move-result-object v6
	   76| aput-object v6, v4, v5
	   78| invoke-virtual {v1,v2,v3}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   81| move-result-object v1
	.line 43
	   82| sget-object v1, java.lang.System.out
	   84| const-string v2, "int            : %s\n"
	   86| const/4 v3, #+1 (0x00000001 | 1.40130e-45)
	   87| new-array v3, v3, java.lang.Object[]
	   89| move-object v8, v3
	   90| move-object v3, v8
	   91| move-object v4, v8
	   92| const/4 v5, #+0 (0x00000000 | 0.00000)
	   93| move-object v6, v0
	   94| const/16 v7, #+100 (0x00000064 | 1.40130e-43)
	   96| invoke-virtual {v6,v7}, Target.testInt(int):int
	   99| move-result v6
	  100| invoke-static {v6}, java.lang.Integer.valueOf(int):java.lang.Integer
	  103| move-result-object v6
	  104| aput-object v6, v4, v5
	  106| invoke-virtual {v1,v2,v3}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	  109| move-result-object v1
	.line 44
	  110| move-object v1, v0
	  111| const/4 v2, #+1 (0x00000001 | 1.40130e-45)
	  112| invoke-virtual {v1,v2}, Target.testVoid(boolean):void
	.line 45
	  115| return-void
}

method Target.testDouble(int):double
{
	.params "?"
	.src "exitHooks.java"
	.line 60
	.prologue_end
	.line 60
	    0| move-object v0, v4
	.local v0, "this", Target
	    1| move v1, v5
	.local v1, "n", int
	    2| move v2, v1
	    3| packed-switch v2, Label_5
	.line 65
	    6| const-wide v2:v3, #+4621762822593629389 (0x4023cccccccccccd | 9.90000)
	   11| move-wide v0:v1, v2:v3
Label_1:
	.end_local v0
	   12| return-wide v0:v1
Label_2:
	.line 62
	.restart_local v0
	   13| const-wide v2:v3, #+4607632778762754458 (0x3ff199999999999a | 1.10000)
	   18| move-wide v0:v1, v2:v3
	   19| goto Label_1
Label_3:
	.line 63
	   20| const-wide v2:v3, #+4612136378390124954 (0x400199999999999a | 2.20000)
	   25| move-wide v0:v1, v2:v3
	   26| goto Label_1
Label_4:
	.line 64
	   27| const-wide v2:v3, #+4614613358185178726 (0x400a666666666666 | 3.30000)
	   32| move-wide v0:v1, v2:v3
	   33| goto Label_1
Label_5: <aligned>
	.line 60
	   34| packed-switch-payload
		    1: Label_2
		    2: Label_3
		    3: Label_4
}

method Target.testInt(int):int
{
	.params "?"
	.src "exitHooks.java"
	.line 71
	.prologue_end
	.line 71
	    0| move-object v0, v3
	.local v0, "this", Target
	    1| move v1, v4
	.local v1, "n", int
	    2| move v2, v1
	    3| sparse-switch v2, Label_5
	.line 76
	    6| const/16 v2, #+123 (0x0000007b | 1.72360e-43)
	    8| move v0, v2
Label_1:
	.end_local v0
	    9| return v0
Label_2:
	.line 73
	.restart_local v0
	   10| const/4 v2, #+1 (0x00000001 | 1.40130e-45)
	   11| move v0, v2
	   12| goto Label_1
Label_3:
	.line 74
	   13| const/4 v2, #+2 (0x00000002 | 2.80260e-45)
	   14| move v0, v2
	   15| goto Label_1
Label_4:
	.line 75
	   16| const/4 v2, #+3 (0x00000003 | 4.20390e-45)
	   17| move v0, v2
	   18| goto Label_1
	.line 71
	   19| nop
Label_5: <aligned>
	   20| sparse-switch-payload
		   10: Label_2
		   20: Label_3
		   30: Label_4
}

method Target.testObject(boolean):java.lang.Object
{
	.params "?"
	.src "exitHooks.java"
	.line 49
	.prologue_end
	.line 49
	    0| move-object v0, v5
	.local v0, "this", Target
	    1| move v1, v6
	.local v1, "flag", boolean
	    2| move v2, v1
	    3| if-eqz v2, Label_2
	.line 51
	    5| sget-object v2, java.lang.System.out
	    7| const-string v3, "Flag!\n"
	    9| const/4 v4, #+0 (0x00000000 | 0.00000)
	   10| new-array v4, v4, java.lang.Object[]
	   12| invoke-virtual {v2,v3,v4}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   15| move-result-object v2
	.line 52
	   16| const-string v2, "Sigh"
	   18| move-object v0, v2
Label_1:
	.line 55
	.end_local v0
	   19| return-object v0
Label_2:
	.restart_local v0
	   20| const-string v2, "Blah"
	   22| move-object v0, v2
	   23| goto Label_1
}

method Target.testVoid(boolean):void
{
	.params "?"
	.src "exitHooks.java"
	.line 82
	.prologue_end
	.line 82
	    0| move-object v0, v5
	.local v0, "this", Target
	    1| move v1, v6
	.local v1, "flag", boolean
	    2| move v2, v1
	    3| if-eqz v2, Label_2
	.line 84
	    5| sget-object v2, java.lang.System.out
	    7| const-string v3, "True!\n"
	    9| const/4 v4, #+0 (0x00000000 | 0.00000)
	   10| new-array v4, v4, java.lang.Object[]
	   12| invoke-virtual {v2,v3,v4}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   15| move-result-object v2
Label_1:
	.line 85
	.line 90
	   16| return-void
Label_2:
	.line 89
	   17| sget-object v2, java.lang.System.out
	   19| const-string v3, "False!\n"
	   21| const/4 v4, #+0 (0x00000000 | 0.00000)
	   22| new-array v4, v4, java.lang.Object[]
	   24| invoke-virtual {v2,v3,v4}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   27| move-result-object v2
	.line 90
	   28| goto Label_1
}

method Tracer.<init>():void
{
	.src "exitHooks.java"
	.line 2
	.prologue_end
	.line 2
	    0| move-object v0, v2
	.local v0, "this", Tracer
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Object.<init>():void
	    5| return-void
}

method Tracer.onExit(double):double
{
	.params "?"
	.src "exitHooks.java"
	.line 12
	.prologue_end
	.line 12
	    0| move-wide v0:v1, v10:v11
	.local v0, "value", double
	    1| sget-object v2, java.lang.System.out
	    3| const-string v3, ">>> onExit(double: %f)\n"
	    5| const/4 v4, #+1 (0x00000001 | 1.40130e-45)
	    6| new-array v4, v4, java.lang.Object[]
	    8| move-object v9, v4
	    9| move-object v4, v9
	   10| move-object v5, v9
	   11| const/4 v6, #+0 (0x00000000 | 0.00000)
	   12| move-wide v7:v8, v0:v1
	   13| invoke-static {v7,v8}, java.lang.Double.valueOf(double):java.lang.Double
	   16| move-result-object v7
	   17| aput-object v7, v5, v6
	   19| invoke-virtual {v2,v3,v4}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   22| move-result-object v2
	.line 13
	   23| move-wide v2:v3, v0:v1
	   24| neg-double v2:v3, v2:v3
	   25| move-wide v0:v1, v2:v3
	.end_local v0
	   26| return-wide v0:v1
}

method Tracer.onExit(int):int
{
	.params "?"
	.src "exitHooks.java"
	.line 18
	.prologue_end
	.line 18
	    0| move v0, v8
	.local v0, "value", int
	    1| sget-object v1, java.lang.System.out
	    3| const-string v2, ">>> onExit(int: %d)\n"
	    5| const/4 v3, #+1 (0x00000001 | 1.40130e-45)
	    6| new-array v3, v3, java.lang.Object[]
	    8| move-object v7, v3
	    9| move-object v3, v7
	   10| move-object v4, v7
	   11| const/4 v5, #+0 (0x00000000 | 0.00000)
	   12| move v6, v0
	   13| invoke-static {v6}, java.lang.Integer.valueOf(int):java.lang.Integer
	   16| move-result-object v6
	   17| aput-object v6, v4, v5
	   19| invoke-virtual {v1,v2,v3}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   22| move-result-object v1
	.line 19
	   23| move v1, v0
	   24| const/16 v2, #+10 (0x0000000a | 1.40130e-44)
	   26| mul-int/lit8 v1, v1, #+10 (0x0000000a | 1.40130e-44)
	   28| move v0, v1
	.end_local v0
	   29| return v0
}

method Tracer.onExit(java.lang.Object):java.lang.Object
{
	.params "?"
	.src "exitHooks.java"
	.line 6
	.prologue_end
	.line 6
	    0| move-object v0, v8
	.local v0, "value", java.lang.Object
	    1| sget-object v1, java.lang.System.out
	    3| const-string v2, ">>> onExit(Object: %s)\n"
	    5| const/4 v3, #+1 (0x00000001 | 1.40130e-45)
	    6| new-array v3, v3, java.lang.Object[]
	    8| move-object v7, v3
	    9| move-object v3, v7
	   10| move-object v4, v7
	   11| const/4 v5, #+0 (0x00000000 | 0.00000)
	   12| move-object v6, v0
	   13| aput-object v6, v4, v5
	   15| invoke-virtual {v1,v2,v3}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   18| move-result-object v1
	.line 7
	   19| move-object v1, v0
	   20| move-object v0, v1
	.end_local v0
	   21| return-object v0
}

method Tracer.onExit():void
{
	.src "exitHooks.java"
	.line 24
	.prologue_end
	.line 24
	    0| sget-object v0, java.lang.System.out
	    2| const-string v1, ">>> onExit(void)\n"
	    4| const/4 v2, #+0 (0x00000000 | 0.00000)
	    5| new-array v2, v2, java.lang.Object[]
	    7| invoke-virtual {v0,v1,v2}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   10| move-result-object v0
	.line 25
	   11| return-void
}
