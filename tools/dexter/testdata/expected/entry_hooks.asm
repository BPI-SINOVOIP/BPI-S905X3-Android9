
method Base.<init>():void
{
	.src "entryHooks.java"
	.line 27
	.prologue_end
	.line 27
	    0| move-object v0, v2
	.local v0, "this", Base
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Object.<init>():void
	    5| return-void
}

method Base.foo(int, java.lang.String):int
{
	.params "?", "?"
	.src "entryHooks.java"
	.line 31
	.prologue_end
	.line 31
	    0| move-object v0, v10
	.local v0, "this", Base
	    1| move v1, v11
	.local v1, "x", int
	    2| move-object v2, v12
	.local v2, "msg", java.lang.String
	    3| sget-object v3, java.lang.System.out
	    5| const-string v4, "Base.foo(%d, '%s')\n"
	    7| const/4 v5, #+2 (0x00000002 | 2.80260e-45)
	    8| new-array v5, v5, java.lang.Object[]
	   10| move-object v9, v5
	   11| move-object v5, v9
	   12| move-object v6, v9
	   13| const/4 v7, #+0 (0x00000000 | 0.00000)
	   14| move v8, v1
	   15| invoke-static {v8}, java.lang.Integer.valueOf(int):java.lang.Integer
	   18| move-result-object v8
	   19| aput-object v8, v6, v7
	   21| move-object v9, v5
	   22| move-object v5, v9
	   23| move-object v6, v9
	   24| const/4 v7, #+1 (0x00000001 | 1.40130e-45)
	   25| move-object v8, v2
	   26| aput-object v8, v6, v7
	   28| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   31| move-result-object v3
	.line 32
	   32| move v3, v1
	   33| move v0, v3
	.end_local v0
	   34| return v0
}

method Derived.<init>():void
{
	.src "entryHooks.java"
	.line 36
	.prologue_end
	.line 36
	    0| move-object v0, v2
	.local v0, "this", Derived
	    1| move-object v1, v0
	    2| invoke-direct {v1}, Base.<init>():void
	    5| return-void
}

method Derived.foo(int, java.lang.String):int
{
	.params "?", "?"
	.src "entryHooks.java"
	.line 40
	.prologue_end
	.line 40
	    0| move-object v0, v10
	.local v0, "this", Derived
	    1| move v1, v11
	.local v1, "x", int
	    2| move-object v2, v12
	.local v2, "msg", java.lang.String
	    3| sget-object v3, java.lang.System.out
	    5| const-string v4, "Derived.foo(%d, '%s')\n"
	    7| const/4 v5, #+2 (0x00000002 | 2.80260e-45)
	    8| new-array v5, v5, java.lang.Object[]
	   10| move-object v9, v5
	   11| move-object v5, v9
	   12| move-object v6, v9
	   13| const/4 v7, #+0 (0x00000000 | 0.00000)
	   14| move v8, v1
	   15| invoke-static {v8}, java.lang.Integer.valueOf(int):java.lang.Integer
	   18| move-result-object v8
	   19| aput-object v8, v6, v7
	   21| move-object v9, v5
	   22| move-object v5, v9
	   23| move-object v6, v9
	   24| const/4 v7, #+1 (0x00000001 | 1.40130e-45)
	   25| move-object v8, v2
	   26| aput-object v8, v6, v7
	   28| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   31| move-result-object v3
	.line 41
	   32| move v3, v1
	   33| const/4 v4, #+2 (0x00000002 | 2.80260e-45)
	   34| mul-int/lit8 v3, v3, #+2 (0x00000002 | 2.80260e-45)
	   36| move v0, v3
	.end_local v0
	   37| return v0
}

method Target.<init>():void
{
	.src "entryHooks.java"
	.line 45
	.prologue_end
	.line 45
	    0| move-object v0, v2
	.local v0, "this", Target
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Object.<init>():void
	    5| return-void
}

method Target.main(java.lang.String[]):void
{
	.params "?"
	.src "entryHooks.java"
	.line 51
	.prologue_end
	.line 51
	    0| move-object v0, v8
	.local v0, "args", java.lang.String[]
	    1| sget-object v1, java.lang.System.out
	    3| const-string v2, "Hello, world!"
	    5| invoke-virtual {v1,v2}, java.io.PrintStream.println(java.lang.String):void
	.line 52
	    8| sget-object v1, java.lang.System.out
	   10| const-string v2, "final = %d\n"
	   12| const/4 v3, #+1 (0x00000001 | 1.40130e-45)
	   13| new-array v3, v3, java.lang.Object[]
	   15| move-object v7, v3
	   16| move-object v3, v7
	   17| move-object v4, v7
	   18| const/4 v5, #+0 (0x00000000 | 0.00000)
	   19| invoke-static {}, Target.test():int
	   22| move-result v6
	   23| invoke-static {v6}, java.lang.Integer.valueOf(int):java.lang.Integer
	   26| move-result-object v6
	   27| aput-object v6, v4, v5
	   29| invoke-virtual {v1,v2,v3}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   32| move-result-object v1
	.line 53
	   33| sget-object v1, java.lang.System.out
	   35| const-string v2, "Good bye!"
	   37| invoke-virtual {v1,v2}, java.io.PrintStream.println(java.lang.String):void
	.line 54
	   40| return-void
}

method Target.test():int
{
	.src "entryHooks.java"
	.line 58
	.prologue_end
	.line 58
	    0| new-instance v1, Target
	    2| move-object v4, v1
	    3| move-object v1, v4
	    4| move-object v2, v4
	    5| invoke-direct {v2}, Target.<init>():void
	    8| move-object v0, v1
	.line 59
	.local v0, "obj", Target
	    9| move-object v1, v0
	   10| new-instance v2, Derived
	   12| move-object v4, v2
	   13| move-object v2, v4
	   14| move-object v3, v4
	   15| invoke-direct {v3}, Derived.<init>():void
	   18| iput-object v2, v1, Target.test
	.line 60
	   20| move-object v1, v0
	   21| const/4 v2, #+3 (0x00000003 | 4.20390e-45)
	   22| const-string v3, "Testing..."
	   24| invoke-virtual {v1,v2,v3}, Target.foo(int, java.lang.String):int
	   27| move-result v1
	   28| move v0, v1
	.end_local v0
	   29| return v0
}

method Target.foo(int):int
{
	.params "?"
	.src "entryHooks.java"
	.line 74
	.prologue_end
	.line 74
	    0| move-object v0, v3
	.local v0, "this", Target
	    1| move v1, v4
	.local v1, "x", int
	    2| const/4 v2, #+1 (0x00000001 | 1.40130e-45)
	    3| move v0, v2
	.end_local v0
	    4| return v0
}

method Target.foo(int, int):int
{
	.params "?", "?"
	.src "entryHooks.java"
	.line 75
	.prologue_end
	.line 75
	    0| move-object v0, v4
	.local v0, "this", Target
	    1| move v1, v5
	.local v1, "x", int
	    2| move v2, v6
	.local v2, "y", int
	    3| const/4 v3, #+2 (0x00000002 | 2.80260e-45)
	    4| move v0, v3
	.end_local v0
	    5| return v0
}

method Target.foo(int, java.lang.String):int
{
	.params "?", "?"
	.src "entryHooks.java"
	.line 65
	.prologue_end
	.line 65
	    0| move-object v0, v9
	.local v0, "this", Target
	    1| move v1, v10
	.local v1, "x", int
	    2| move-object v2, v11
	.local v2, "msg", java.lang.String
	    3| const/4 v5, #+0 (0x00000000 | 0.00000)
	    4| move v3, v5
	.line 66
	.local v3, "sum", int
	    5| const/4 v5, #+0 (0x00000000 | 0.00000)
	    6| move v4, v5
Label_1:
	.local v4, "i", int
	    7| move v5, v4
	    8| move v6, v1
	    9| if-ge v5, v6, Label_2
	.line 68
	   11| move v5, v3
	   12| move-object v6, v0
	   13| iget-object v6, v6, Target.test
	   15| move v7, v4
	   16| move-object v8, v2
	   17| invoke-virtual {v6,v7,v8}, Base.foo(int, java.lang.String):int
	   20| move-result v6
	   21| add-int/2addr v5, v6
	   22| move v3, v5
	.line 66
	   23| add-int/lit8 v4, v4, #+1 (0x00000001 | 1.40130e-45)
	   25| goto Label_1
Label_2:
	.line 70
	   26| move v5, v3
	   27| move v0, v5
	.end_local v0
	   28| return v0
}

method Target.foo(int, java.lang.String, java.lang.String):int
{
	.params "?", "?", "?"
	.src "entryHooks.java"
	.line 76
	.prologue_end
	.line 76
	    0| move-object v0, v5
	.local v0, "this", Target
	    1| move v1, v6
	.local v1, "x", int
	    2| move-object v2, v7
	.local v2, "msg", java.lang.String
	    3| move-object v3, v8
	.local v3, "msg2", java.lang.String
	    4| const/4 v4, #+3 (0x00000003 | 4.20390e-45)
	    5| move v0, v4
	.end_local v0
	    6| return v0
}

method Target.foo(int, java.lang.String[]):int
{
	.params "?", "?"
	.src "entryHooks.java"
	.line 77
	.prologue_end
	.line 77
	    0| move-object v0, v4
	.local v0, "this", Target
	    1| move v1, v5
	.local v1, "x", int
	    2| move-object v2, v6
	.local v2, "msgs", java.lang.String[]
	    3| const/4 v3, #+4 (0x00000004 | 5.60519e-45)
	    4| move v0, v3
	.end_local v0
	    5| return v0
}

method Target.foo(int, java.lang.String[][]):java.lang.Integer
{
	.params "?", "?"
	.src "entryHooks.java"
	.line 78
	.prologue_end
	.line 78
	    0| move-object v0, v4
	.local v0, "this", Target
	    1| move v1, v5
	.local v1, "x", int
	    2| move-object v2, v6
	.local v2, "msgs", java.lang.String[][]
	    3| const/4 v3, #+5 (0x00000005 | 7.00649e-45)
	    4| invoke-static {v3}, java.lang.Integer.valueOf(int):java.lang.Integer
	    7| move-result-object v3
	    8| move-object v0, v3
	.end_local v0
	    9| return-object v0
}

method Target.foo():void
{
	.src "entryHooks.java"
	.line 73
	.prologue_end
	.line 73
	    0| return-void
}

method Tracer.<init>():void
{
	.src "entryHooks.java"
	.line 2
	.prologue_end
	.line 2
	    0| move-object v0, v2
	.local v0, "this", Tracer
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Object.<init>():void
	    5| return-void
}

method Tracer.onEntry(java.lang.String):void
{
	.params "?"
	.src "entryHooks.java"
	.line 6
	.prologue_end
	.line 6
	    0| move-object v0, v5
	.local v0, "methodName", java.lang.String
	    1| sget-object v1, java.lang.System.out
	    3| new-instance v2, java.lang.StringBuilder
	    5| move-object v4, v2
	    6| move-object v2, v4
	    7| move-object v3, v4
	    8| invoke-direct {v3}, java.lang.StringBuilder.<init>():void
	   11| const-string v3, "OnEntry("
	   13| invoke-virtual {v2,v3}, java.lang.StringBuilder.append(java.lang.String):java.lang.StringBuilder
	   16| move-result-object v2
	   17| move-object v3, v0
	   18| invoke-virtual {v2,v3}, java.lang.StringBuilder.append(java.lang.String):java.lang.StringBuilder
	   21| move-result-object v2
	   22| const-string v3, ")"
	   24| invoke-virtual {v2,v3}, java.lang.StringBuilder.append(java.lang.String):java.lang.StringBuilder
	   27| move-result-object v2
	   28| invoke-virtual {v2}, java.lang.StringBuilder.toString():java.lang.String
	   31| move-result-object v2
	   32| invoke-virtual {v1,v2}, java.io.PrintStream.println(java.lang.String):void
	.line 7
	   35| return-void
}

method Tracer.onFooEntry(Target, int, java.lang.String):void
{
	.params "?", "?", "?"
	.src "entryHooks.java"
	.line 11
	.prologue_end
	.line 11
	    0| move-object v0, v10
	.local v0, "__this", Target
	    1| move v1, v11
	.local v1, "x", int
	    2| move-object v2, v12
	.local v2, "msg", java.lang.String
	    3| sget-object v3, java.lang.System.out
	    5| const-string v4, ">>> onFooEntry(%s, %d, %s)\n"
	    7| const/4 v5, #+3 (0x00000003 | 4.20390e-45)
	    8| new-array v5, v5, java.lang.Object[]
	   10| move-object v9, v5
	   11| move-object v5, v9
	   12| move-object v6, v9
	   13| const/4 v7, #+0 (0x00000000 | 0.00000)
	   14| move-object v8, v0
	   15| aput-object v8, v6, v7
	   17| move-object v9, v5
	   18| move-object v5, v9
	   19| move-object v6, v9
	   20| const/4 v7, #+1 (0x00000001 | 1.40130e-45)
	   21| move v8, v1
	   22| invoke-static {v8}, java.lang.Integer.valueOf(int):java.lang.Integer
	   25| move-result-object v8
	   26| aput-object v8, v6, v7
	   28| move-object v9, v5
	   29| move-object v5, v9
	   30| move-object v6, v9
	   31| const/4 v7, #+2 (0x00000002 | 2.80260e-45)
	   32| move-object v8, v2
	   33| aput-object v8, v6, v7
	   35| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   38| move-result-object v3
	.line 12
	   39| return-void
}

method Tracer.onFooExit(int):int
{
	.params "?"
	.src "entryHooks.java"
	.line 16
	.prologue_end
	.line 16
	    0| move v0, v8
	.local v0, "retValue", int
	    1| sget-object v1, java.lang.System.out
	    3| const-string v2, ">>> onFooExit(%d)\n"
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
	.line 17
	   23| move v1, v0
	   24| const/16 v2, #+100 (0x00000064 | 1.40130e-43)
	   26| add-int/lit8 v1, v1, #+100 (0x00000064 | 1.40130e-43)
	   28| move v0, v1
	.end_local v0
	   29| return v0
}

method Tracer.wrapFoo(Base, int, java.lang.String):int
{
	.params "?", "?", "?"
	.src "entryHooks.java"
	.line 22
	.prologue_end
	.line 22
	    0| move-object v0, v10
	.local v0, "_this", Base
	    1| move v1, v11
	.local v1, "x", int
	    2| move-object v2, v12
	.local v2, "msg", java.lang.String
	    3| sget-object v3, java.lang.System.out
	    5| const-string v4, ">>> %s.test(%d, %s)\n"
	    7| const/4 v5, #+3 (0x00000003 | 4.20390e-45)
	    8| new-array v5, v5, java.lang.Object[]
	   10| move-object v9, v5
	   11| move-object v5, v9
	   12| move-object v6, v9
	   13| const/4 v7, #+0 (0x00000000 | 0.00000)
	   14| move-object v8, v0
	   15| invoke-virtual {v8}, java.lang.Object.getClass():java.lang.Class
	   18| move-result-object v8
	   19| invoke-virtual {v8}, java.lang.Class.getName():java.lang.String
	   22| move-result-object v8
	   23| aput-object v8, v6, v7
	   25| move-object v9, v5
	   26| move-object v5, v9
	   27| move-object v6, v9
	   28| const/4 v7, #+1 (0x00000001 | 1.40130e-45)
	   29| move v8, v1
	   30| invoke-static {v8}, java.lang.Integer.valueOf(int):java.lang.Integer
	   33| move-result-object v8
	   34| aput-object v8, v6, v7
	   36| move-object v9, v5
	   37| move-object v5, v9
	   38| move-object v6, v9
	   39| const/4 v7, #+2 (0x00000002 | 2.80260e-45)
	   40| move-object v8, v2
	   41| aput-object v8, v6, v7
	   43| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   46| move-result-object v3
	.line 23
	   47| move-object v3, v0
	   48| move v4, v1
	   49| move-object v5, v2
	   50| invoke-virtual {v3,v4,v5}, Base.foo(int, java.lang.String):int
	   53| move-result v3
	   54| const/16 v4, #+10 (0x0000000a | 1.40130e-44)
	   56| add-int/lit8 v3, v3, #+10 (0x0000000a | 1.40130e-44)
	   58| move v0, v3
	.end_local v0
	   59| return v0
}
