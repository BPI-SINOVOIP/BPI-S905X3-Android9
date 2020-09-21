
method A.<init>():void
{
	.src "tryCatch.java"
	.line 2
	.prologue_end
	.line 2
	    0| move-object v0, v2
	.local v0, "this", A
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Throwable.<init>():void
	    5| return-void
}

method B.<init>():void
{
	.src "tryCatch.java"
	.line 6
	.prologue_end
	.line 6
	    0| move-object v0, v2
	.local v0, "this", B
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Throwable.<init>():void
	    5| return-void
}

method TryCatch.<init>():void
{
	.src "tryCatch.java"
	.line 10
	.prologue_end
	.line 10
	    0| move-object v0, v2
	.local v0, "this", TryCatch
	    1| move-object v1, v0
	    2| invoke-direct {v1}, java.lang.Object.<init>():void
	    5| return-void
}

method TryCatch.foo(int):int
{
	.params "?"
	.src "tryCatch.java"
	.line 27
	.prologue_end
	.line 27
	    0| move v0, v4
	.local v0, "x", int
	    1| move v1, v0
	    2| if-lez v1, Label_4
	.line 28
	    4| move v1, v0
	    5| packed-switch v1, Label_5
	.line 31
	    8| const/4 v1, #+1 (0x00000001 | 1.40130e-45)
	    9| move v0, v1
Label_1:
	.line 34
	.end_local v0
	   10| return v0
Label_2:
	.line 29
	.restart_local v0
	   11| new-instance v1, A
	   13| move-object v3, v1
	   14| move-object v1, v3
	   15| move-object v2, v3
	   16| invoke-direct {v2}, A.<init>():void
	   19| throw v1
Label_3:
	.line 30
	   20| new-instance v1, B
	   22| move-object v3, v1
	   23| move-object v1, v3
	   24| move-object v2, v3
	   25| invoke-direct {v2}, B.<init>():void
	   28| throw v1
Label_4:
	.line 34
	   29| const/4 v1, #+0 (0x00000000 | 0.00000)
	   30| move v0, v1
	   31| goto Label_1
Label_5: <aligned>
	.line 28
	   32| packed-switch-payload
		    1: Label_2
		    2: Label_3
}

method TryCatch.main(java.lang.String[]):void
{
	.params "?"
	.src "tryCatch.java"
	.line 15
	.prologue_end
	.line 15
	    0| move-object v0, v6
	.local v0, "args", java.lang.String[]
	    1| const/4 v3, #+0 (0x00000000 | 0.00000)
	.try_begin_1
	    2| invoke-static {v3}, TryCatch.foo(int):int
	.try_end_1
	  catch(B) : Label_3
	  catch(A) : Label_4
	  catch(...) : Label_5
	    5| move-result v3
Label_1:
	.line 18
	.line 22
	    6| sget-object v3, java.lang.System.out
	    8| const-string v4, "finally\n"
	   10| const/4 v5, #+0 (0x00000000 | 0.00000)
	   11| new-array v5, v5, java.lang.Object[]
	   13| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   16| move-result-object v3
Label_2:
	.line 23
	.line 24
	   17| return-void
Label_3:
	.line 16
	   18| move-exception v3
	   19| move-object v1, v3
	.try_begin_2
	.line 17
	.local v1, "ex", B
	   20| sget-object v3, java.lang.System.out
	   22| const-string v4, "catch: B\n"
	   24| const/4 v5, #+0 (0x00000000 | 0.00000)
	   25| new-array v5, v5, java.lang.Object[]
	   27| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	.try_end_2
	  catch(A) : Label_4
	  catch(...) : Label_5
	   30| move-result-object v3
	   31| goto Label_1
Label_4:
	.line 19
	.end_local v1
	   32| move-exception v3
	   33| move-object v1, v3
	.try_begin_3
	.line 20
	.local v1, "ex", A
	   34| sget-object v3, java.lang.System.out
	   36| const-string v4, "catch: A\n"
	   38| const/4 v5, #+0 (0x00000000 | 0.00000)
	   39| new-array v5, v5, java.lang.Object[]
	   41| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	.try_end_3
	  catch(...) : Label_5
	   44| move-result-object v3
	.line 22
	   45| sget-object v3, java.lang.System.out
	   47| const-string v4, "finally\n"
	   49| const/4 v5, #+0 (0x00000000 | 0.00000)
	   50| new-array v5, v5, java.lang.Object[]
	   52| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   55| move-result-object v3
	.line 23
	   56| goto Label_2
Label_5:
	.line 22
	.end_local v1
	   57| move-exception v3
	   58| move-object v2, v3
	   59| sget-object v3, java.lang.System.out
	   61| const-string v4, "finally\n"
	   63| const/4 v5, #+0 (0x00000000 | 0.00000)
	   64| new-array v5, v5, java.lang.Object[]
	   66| invoke-virtual {v3,v4,v5}, java.io.PrintStream.printf(java.lang.String, java.lang.Object[]):java.io.PrintStream
	   69| move-result-object v3
	   70| move-object v3, v2
	   71| throw v3
}
