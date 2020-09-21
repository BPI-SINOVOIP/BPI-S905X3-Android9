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
package dagger.internal.codegen.writer;

import com.google.common.collect.Iterables;
import com.google.common.collect.Lists;
import dagger.internal.codegen.writer.Writable.Context;
import java.io.IOException;
import java.lang.annotation.Annotation;
import java.util.EnumSet;
import java.util.List;
import java.util.Set;
import javax.lang.model.element.Modifier;

public abstract class Modifiable {
  final Set<Modifier> modifiers;
  final List<AnnotationWriter> annotations;

  Modifiable() {
    this.modifiers = EnumSet.noneOf(Modifier.class);
    this.annotations = Lists.newArrayList();
  }

  public void addModifiers(Modifier first, Modifier... rest) {
    addModifiers(Lists.asList(first, rest));
  }

  public void addModifiers(Iterable<Modifier> modifiers) {
    Iterables.addAll(this.modifiers, modifiers);
  }

  public AnnotationWriter annotate(ClassName annotation) {
    AnnotationWriter annotationWriter = new AnnotationWriter(annotation);
    this.annotations.add(annotationWriter);
    return annotationWriter;
  }

  public AnnotationWriter annotate(Class<? extends Annotation> annotation) {
    return annotate(ClassName.fromClass(annotation));
  }

  Appendable writeModifiers(Appendable appendable) throws IOException {
    for (Modifier modifier : modifiers) {
      appendable.append(modifier.toString()).append(' ');
    }
    return appendable;
  }

  Appendable writeAnnotations(Appendable appendable, Context context) throws IOException {
    for (AnnotationWriter annotationWriter : annotations) {
      annotationWriter.write(appendable, context).append('\n');
    }
    return appendable;
  }
}
