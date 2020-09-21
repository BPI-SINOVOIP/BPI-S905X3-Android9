/*
 * Copyright (C) 2014 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package dagger.internal.codegen;

import com.google.auto.common.BasicAnnotationProcessor.ProcessingStep;
import com.google.auto.common.MoreElements;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Maps;
import com.google.common.collect.SetMultimap;
import dagger.Component;
import dagger.Subcomponent;
import dagger.internal.codegen.ComponentDescriptor.Factory;
import dagger.internal.codegen.ComponentValidator.ComponentValidationReport;
import java.lang.annotation.Annotation;
import java.util.Map;
import java.util.Set;
import javax.annotation.processing.Messager;
import javax.lang.model.element.Element;
import javax.lang.model.element.TypeElement;

/**
 * A {@link ProcessingStep} that is responsible for dealing with the {@link Component} annotation
 * as part of the {@link ComponentProcessor}.
 *
 * @author Gregory Kick
 */
final class ComponentProcessingStep extends AbstractComponentProcessingStep {
  private final Messager messager;
  private final ComponentValidator componentValidator;
  private final ComponentValidator subcomponentValidator;
  private final BuilderValidator componentBuilderValidator;
  private final BuilderValidator subcomponentBuilderValidator;

  ComponentProcessingStep(
      Messager messager,
      ComponentValidator componentValidator,
      ComponentValidator subcomponentValidator,
      BuilderValidator componentBuilderValidator,
      BuilderValidator subcomponentBuilderValidator,
      ComponentHierarchyValidator componentHierarchyValidator,
      BindingGraphValidator bindingGraphValidator,
      Factory componentDescriptorFactory,
      BindingGraph.Factory bindingGraphFactory,
      ComponentGenerator componentGenerator) {
    super(
        Component.class,
        messager,
        componentHierarchyValidator,
        bindingGraphValidator,
        componentDescriptorFactory,
        bindingGraphFactory,
        componentGenerator);
    this.messager = messager;
    this.componentValidator = componentValidator;
    this.subcomponentValidator = subcomponentValidator;
    this.componentBuilderValidator = componentBuilderValidator;
    this.subcomponentBuilderValidator = subcomponentBuilderValidator;
  }

  @Override
  public Set<Class<? extends Annotation>> annotations() {
    return ImmutableSet.<Class<? extends Annotation>>of(Component.class, Component.Builder.class,
        Subcomponent.class, Subcomponent.Builder.class);
  }

  @Override
  protected ComponentElementValidator componentElementValidator(
      SetMultimap<Class<? extends Annotation>, Element> elementsByAnnotation) {
    final Map<Element, ValidationReport<TypeElement>> builderReportsByComponent =
        processComponentBuilders(elementsByAnnotation.get(Component.Builder.class));
    final Set<Element> subcomponentBuilderElements =
        elementsByAnnotation.get(Subcomponent.Builder.class);
    final Map<Element, ValidationReport<TypeElement>> builderReportsBySubcomponent =
        processSubcomponentBuilders(subcomponentBuilderElements);
    final Set<Element> subcomponentElements = elementsByAnnotation.get(Subcomponent.class);
    final Map<Element, ValidationReport<TypeElement>> reportsBySubcomponent =
        processSubcomponents(subcomponentElements, subcomponentBuilderElements);
    return new ComponentElementValidator() {
      @Override
      boolean validateComponent(TypeElement componentTypeElement, Messager messager) {
        ComponentValidationReport validationReport =
            componentValidator.validate(
                componentTypeElement, subcomponentElements, subcomponentBuilderElements);
        validationReport.report().printMessagesTo(messager);
        return isClean(
            validationReport,
            builderReportsByComponent,
            reportsBySubcomponent,
            builderReportsBySubcomponent);
      }
    };
  }

  private Map<Element, ValidationReport<TypeElement>> processComponentBuilders(
      Set<? extends Element> componentBuilderElements) {
    Map<Element, ValidationReport<TypeElement>> builderReportsByComponent = Maps.newHashMap();
    for (Element element : componentBuilderElements) {
      ValidationReport<TypeElement> report =
          componentBuilderValidator.validate(MoreElements.asType(element));
      report.printMessagesTo(messager);
      builderReportsByComponent.put(element.getEnclosingElement(), report);
    }
    return builderReportsByComponent;
  }

  private Map<Element, ValidationReport<TypeElement>> processSubcomponentBuilders(
      Set<? extends Element> subcomponentBuilderElements) {
    Map<Element, ValidationReport<TypeElement>> builderReportsBySubcomponent = Maps.newHashMap();
    for (Element element : subcomponentBuilderElements) {
      ValidationReport<TypeElement> report =
          subcomponentBuilderValidator.validate(MoreElements.asType(element));
      report.printMessagesTo(messager);
      builderReportsBySubcomponent.put(element, report);
    }
    return builderReportsBySubcomponent;
  }

  private Map<Element, ValidationReport<TypeElement>> processSubcomponents(
      Set<? extends Element> subcomponentElements,
      Set<? extends Element> subcomponentBuilderElements) {
    Map<Element, ValidationReport<TypeElement>> reportsBySubcomponent = Maps.newHashMap();
    for (Element element : subcomponentElements) {
      ComponentValidationReport report = subcomponentValidator.validate(
          MoreElements.asType(element), subcomponentElements, subcomponentBuilderElements);
      report.report().printMessagesTo(messager);
      reportsBySubcomponent.put(element, report.report());
    }
    return reportsBySubcomponent;
  }

  /**
   * Returns true if the component's report is clean, its builder report is clean, and all
   * referenced subcomponent reports & subcomponent builder reports are clean.
   */
  private boolean isClean(ComponentValidationReport report,
      Map<Element, ValidationReport<TypeElement>> builderReportsByComponent,
      Map<Element, ValidationReport<TypeElement>> reportsBySubcomponent,
      Map<Element, ValidationReport<TypeElement>> builderReportsBySubcomponent) {
    Element component = report.report().subject();
    ValidationReport<?> componentReport = report.report();
    if (!componentReport.isClean()) {
      return false;
    }
    ValidationReport<?> builderReport = builderReportsByComponent.get(component);
    if (builderReport != null && !builderReport.isClean()) {
      return false;
    }
    for (Element element : report.referencedSubcomponents()) {
      ValidationReport<?> subcomponentBuilderReport = builderReportsBySubcomponent.get(element);
      if (subcomponentBuilderReport != null && !subcomponentBuilderReport.isClean()) {
        return false;
      }
      ValidationReport<?> subcomponentReport = reportsBySubcomponent.get(element);
      if (subcomponentReport != null && !subcomponentReport.isClean()) {
        return false;
      }
    }
    return true;
  }
}
