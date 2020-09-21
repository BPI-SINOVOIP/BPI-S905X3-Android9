A Java-language API for doing compile time or runtime code generation targeting the Dalvik VM. Unlike
[cglib](http://cglib.sourceforge.net/) or [ASM](http://asm.ow2.org/), this library creates Dalvik `.dex`
files instead of Java `.class` files.

It has a small, close-to-the-metal API. This API mirrors the
[Dalvik bytecode specification](http://source.android.com/tech/dalvik/dalvik-bytecode.html) giving you tight
control over the bytecode emitted. Code is generated instruction-by-instruction; you bring your own abstract
syntax tree if you need one. And since it uses Dalvik's `dx` tool as a backend, you get efficient register
allocation and regular/wide instruction selection for free.

Class Proxies
-------------

Dexmaker includes a stock code generator for [class proxies](http://dexmaker.googlecode.com/git/javadoc/com/google/dexmaker/stock/ProxyBuilder.html).
If you just want to do AOP or class mocking, you don't need to mess around with bytecodes.

Mockito Mocks
-------------

Dexmaker includes class proxy support for [Mockito](http://code.google.com/p/mockito/). Add the mockito
and the dexmaker `.jar` files to your Android test project's `libs/` directory and you can use Mockito
in your Android unit tests.

This requires Mockito 1.10.5 or newer.

Runtime Code Generation
-----------------------

This example generates a class and a method. It then loads that class into the current process and invokes its method.
```
public final class HelloWorldMaker {
    public static void main(String[] args) throws Exception {
        DexMaker dexMaker = new DexMaker();

        // Generate a HelloWorld class.
        TypeId<?> helloWorld = TypeId.get("LHelloWorld;");
        dexMaker.declare(helloWorld, "HelloWorld.generated", Modifier.PUBLIC, TypeId.OBJECT);
        generateHelloMethod(dexMaker, helloWorld);

        // Create the dex file and load it.
        File outputDir = new File(".");
        ClassLoader loader = dexMaker.generateAndLoad(HelloWorldMaker.class.getClassLoader(),
                outputDir, outputDir);
        Class<?> helloWorldClass = loader.loadClass("HelloWorld");

        // Execute our newly-generated code in-process.
        helloWorldClass.getMethod("hello").invoke(null);
    }

    /**
     * Generates Dalvik bytecode equivalent to the following method.
     *    public static void hello() {
     *        int a = 0xabcd;
     *        int b = 0xaaaa;
     *        int c = a - b;
     *        String s = Integer.toHexString(c);
     *        System.out.println(s);
     *        return;
     *    }
     */
    private static void generateHelloMethod(DexMaker dexMaker, TypeId<?> declaringType) {
        // Lookup some types we'll need along the way.
        TypeId<System> systemType = TypeId.get(System.class);
        TypeId<PrintStream> printStreamType = TypeId.get(PrintStream.class);

        // Identify the 'hello()' method on declaringType.
        MethodId hello = declaringType.getMethod(TypeId.VOID, "hello");

        // Declare that method on the dexMaker. Use the returned Code instance
        // as a builder that we can append instructions to.
        Code code = dexMaker.declare(hello, Modifier.STATIC | Modifier.PUBLIC);

        // Declare all the locals we'll need up front. The API requires this.
        Local<Integer> a = code.newLocal(TypeId.INT);
        Local<Integer> b = code.newLocal(TypeId.INT);
        Local<Integer> c = code.newLocal(TypeId.INT);
        Local<String> s = code.newLocal(TypeId.STRING);
        Local<PrintStream> localSystemOut = code.newLocal(printStreamType);

        // int a = 0xabcd;
        code.loadConstant(a, 0xabcd);

        // int b = 0xaaaa;
        code.loadConstant(b, 0xaaaa);

        // int c = a - b;
        code.op(BinaryOp.SUBTRACT, c, a, b);

        // String s = Integer.toHexString(c);
        MethodId<Integer, String> toHexString
                = TypeId.get(Integer.class).getMethod(TypeId.STRING, "toHexString", TypeId.INT);
        code.invokeStatic(toHexString, s, c);

        // System.out.println(s);
        FieldId<System, PrintStream> systemOutField = systemType.getField(printStreamType, "out");
        code.sget(systemOutField, localSystemOut);
        MethodId<PrintStream, Void> printlnMethod = printStreamType.getMethod(
                TypeId.VOID, "println", TypeId.STRING);
        code.invokeVirtual(printlnMethod, null, localSystemOut, s);

        // return;
        code.returnVoid();
    }
}
```

Use it in your app
------------------

Maven users can get dexmaker from Sonatype's central repository. The Mockito dependency is optional.

```
  <dependency>
    <groupId>com.google.dexmaker</groupId>
    <artifactId>dexmaker</artifactId>
    <version>1.2</version>
  </dependency>
  <dependency>
    <groupId>com.google.dexmaker</groupId>
    <artifactId>dexmaker-mockito</artifactId>
    <version>1.2</version>
  </dependency>
```

Download [dexmaker-1.2.jar](http://search.maven.org/remotecontent?filepath=com/google/dexmaker/dexmaker/1.2/dexmaker-1.2.jar)
and [dexmaker-mockito-1.2.jar](http://search.maven.org/remotecontent?filepath=com/google/dexmaker/dexmaker-mockito/1.2/dexmaker-mockito-1.2.jar).

Run the Unit Tests
------------------

The unit tests for dexmaker must be run on a dalvikvm. In order to do this, you can use [Vogar](https://code.google.com/p/vogar/) in the following fashion:

```
$ java -jar vogar.jar --mode device --sourcepath /path/to/dexmaker/dexmaker/src/test/java --sourcepath /path/to/dexmaker/dexmaker/src/main/java --sourcepath /path/to/dexmaker/dx/src/main/java --device-dir /data/dexmaker /path/to/dexmaker/dexmaker/src/test/
```

Download [vogar.jar](https://vogar.googlecode.com/files/vogar.jar).
