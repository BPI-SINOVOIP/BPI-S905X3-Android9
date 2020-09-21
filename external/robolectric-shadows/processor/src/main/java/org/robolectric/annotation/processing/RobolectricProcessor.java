package org.robolectric.annotation.processing;

import com.google.common.annotations.VisibleForTesting;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.ProcessingEnvironment;
import javax.annotation.processing.RoundEnvironment;
import javax.annotation.processing.SupportedAnnotationTypes;
import javax.annotation.processing.SupportedOptions;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.TypeElement;
import org.robolectric.annotation.processing.generator.Generator;
import org.robolectric.annotation.processing.generator.JavadocJsonGenerator;
import org.robolectric.annotation.processing.generator.ServiceLoaderGenerator;
import org.robolectric.annotation.processing.generator.ShadowProviderGenerator;
import org.robolectric.annotation.processing.validator.ImplementationValidator;
import org.robolectric.annotation.processing.validator.ImplementsValidator;
import org.robolectric.annotation.processing.validator.RealObjectValidator;
import org.robolectric.annotation.processing.validator.ResetterValidator;
import org.robolectric.annotation.processing.validator.Validator;

/**
 * Annotation processor entry point for Robolectric annotations.
 */
@SupportedOptions({
  RobolectricProcessor.PACKAGE_OPT, 
  RobolectricProcessor.SHOULD_INSTRUMENT_PKG_OPT})
@SupportedAnnotationTypes("org.robolectric.annotation.*")
public class RobolectricProcessor extends AbstractProcessor {
  static final String PACKAGE_OPT = "org.robolectric.annotation.processing.shadowPackage";
  static final String SHOULD_INSTRUMENT_PKG_OPT = 
      "org.robolectric.annotation.processing.shouldInstrumentPackage";
  
  private RobolectricModel model;
  private String shadowPackage;
  private boolean shouldInstrumentPackages;
  private Map<String, String> options;
  private boolean generated = false;
  private final List<Generator> generators = new ArrayList<>();
  private final Map<TypeElement, Validator> elementValidators = new HashMap<>(13);

  /**
   * Default constructor.
   */
  public RobolectricProcessor() {
  }

  /**
   * Constructor to use for testing passing options in. Only
   * necessary until compile-testing supports passing options
   * in.
   *
   * @param options simulated options that would ordinarily
   *                be passed in the {@link ProcessingEnvironment}.
   */
  @VisibleForTesting
  public RobolectricProcessor(Map<String, String> options) {
    processOptions(options);
  }

  @Override
  public synchronized void init(ProcessingEnvironment environment) {
    super.init(environment);
    processOptions(environment.getOptions());
    model = new RobolectricModel(environment.getElementUtils(), environment.getTypeUtils());

    addValidator(new ImplementationValidator(model, environment));
    addValidator(new ImplementsValidator(model, environment));
    addValidator(new RealObjectValidator(model, environment));
    addValidator(new ResetterValidator(model, environment));

    generators.add(new ShadowProviderGenerator(model, environment, shadowPackage, shouldInstrumentPackages));
    generators.add(new ServiceLoaderGenerator(environment, shadowPackage));
    generators.add(new JavadocJsonGenerator(model, environment));
  }

  @Override
  public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
    for (TypeElement annotation : annotations) {
      Validator validator = elementValidators.get(annotation);
      if (validator != null) {
        for (Element elem : roundEnv.getElementsAnnotatedWith(annotation)) {
          validator.visit(elem, elem.getEnclosingElement());
        }
      }
    }

    if (!generated) {
      model.prepare();
      for (Generator generator : generators) {
        generator.generate();
      }
      generated = true;
    }
    return true;
  }

  private void addValidator(Validator v) {
    elementValidators.put(v.getAnnotationType(), v);
  }

  private void processOptions(Map<String, String> options) {
    if (this.options == null) {
      this.options = options;
      this.shadowPackage = options.get(PACKAGE_OPT);
      this.shouldInstrumentPackages =
          !"false".equalsIgnoreCase(options.get(SHOULD_INSTRUMENT_PKG_OPT));

      if (this.shadowPackage == null) {
        throw new IllegalArgumentException("no package specified for " + PACKAGE_OPT);
      }
    }
  }

  @Override
  public SourceVersion getSupportedSourceVersion() {
    return SourceVersion.latest();
  }
}
