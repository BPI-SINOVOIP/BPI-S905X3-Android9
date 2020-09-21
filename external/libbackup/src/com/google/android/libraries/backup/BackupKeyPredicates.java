package com.google.android.libraries.backup;

import android.content.Context;
import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Static utility methods returning {@link BackupKeyPredicate} instances. */
public class BackupKeyPredicates {

  /**
   * Returns a predicate that determines whether a key was defined as a field with the given
   * annotation in one of the given classes. Assumes that the given annotation and classes are
   * valid. You must ensure that proguard does not remove your annotation or any fields annotated
   * with it.
   *
   * @see Backup
   */
  public static BackupKeyPredicate buildPredicateFromAnnotatedFieldsIn(
      Class<? extends Annotation> annotation, Class<?>... klasses) {
    return in(getAnnotatedFieldValues(annotation, klasses));
  }

  /**
   * Returns a predicate that determines whether a key matches a regex that was defined as a field
   * with the given annotation in one of the given classes. The test used is equivalent to
   * {@link #containsPattern(String)} for each annotated field value. Assumes that the given
   * annotation and classes are valid. You must ensure that proguard does not remove your annotation
   * or any fields annotated with it.
   *
   * @see Backup
   */
  public static BackupKeyPredicate buildPredicateFromAnnotatedRegexFieldsIn(
      Class<? extends Annotation> annotation, Class<?>... klasses) {
    Set<String> patterns = getAnnotatedFieldValues(annotation, klasses);
    Set<BackupKeyPredicate> patternPredicates = new HashSet<>();
    for (String pattern : patterns) {
      patternPredicates.add(containsPattern(pattern));
    }
    return or(patternPredicates);
  }

  private static Set<String> getAnnotatedFieldValues(
      Class<? extends Annotation> annotation, Class<?>... klasses) {
    Set<String> values = new HashSet<>();
    for (Class<?> klass : klasses) {
      addAnnotatedFieldValues(annotation, klass, values);
    }
    return values;
  }

  private static void addAnnotatedFieldValues(
      Class<? extends Annotation> annotation, Class<?> klass, Set<String> values) {
    for (Field field : klass.getDeclaredFields()) {
      addFieldValueIfAnnotated(annotation, field, values);
    }
  }

  private static void addFieldValueIfAnnotated(
      Class<? extends Annotation> annotation, Field field, Set<String> values) {
    if (field.isAnnotationPresent(annotation) && field.getType().equals(String.class)) {
      try {
        values.add((String) field.get(null));
      } catch (IllegalAccessException e) {
        throw new IllegalArgumentException(e);
      }
    }
  }

  /**
   * Returns a predicate that determines whether a key is a member of the given collection. Changes
   * to the given collection will change the returned predicate.
   */
  public static BackupKeyPredicate in(final Collection<? extends String> collection) {
    if (collection == null) {
      throw new NullPointerException("Null collection given.");
    }
    return new BackupKeyPredicate() {
      @Override
      public boolean shouldBeBackedUp(String key) {
        return collection.contains(key);
      }
    };
  }

  /**
   * Returns a predicate that determines whether a key contains any match for the given regular
   * expression pattern. The test used is equivalent to {@link Matcher#find()}.
   */
  public static BackupKeyPredicate containsPattern(String pattern) {
    final Pattern compiledPattern = Pattern.compile(pattern);
    return new BackupKeyPredicate() {
      @Override
      public boolean shouldBeBackedUp(String key) {
        return compiledPattern.matcher(key).find();
      }
    };
  }

  /**
   * Returns a predicate that determines whether a key passes any of the given predicates. Each
   * predicate is evaluated in the order given, and the evaluation process stops as soon as an
   * accepting predicate is found. Changes to the given iterable will not change the returned
   * predicate. The returned predicate returns {@code false} for any key if the given iterable is
   * empty.
   */
  public static BackupKeyPredicate or(Iterable<BackupKeyPredicate> predicates) {
    final List<BackupKeyPredicate> copiedPredicates = new ArrayList<>();
    for (BackupKeyPredicate predicate : predicates) {
      copiedPredicates.add(predicate);
    }
    return orDefensivelyCopied(new ArrayList<>(copiedPredicates));
  }

  /**
   * Returns a predicate that determines whether a key passes any of the given predicates. Each
   * predicate is evaluated in the order given, and the evaluation process stops as soon as an
   * accepting predicate is found. The returned predicate returns {@code false} for any key if no
   * there are no given predicates.
   */
  public static BackupKeyPredicate or(BackupKeyPredicate... predicates) {
    return orDefensivelyCopied(Arrays.asList(predicates));
  }

  private static BackupKeyPredicate orDefensivelyCopied(
      final Iterable<BackupKeyPredicate> predicates) {
    return new BackupKeyPredicate() {
      @Override
      public boolean shouldBeBackedUp(String key) {
        for (BackupKeyPredicate predicate : predicates) {
          if (predicate.shouldBeBackedUp(key)) {
            return true;
          }
        }
        return false;
      }
    };
  }

  /**
   * Returns a predicate that determines whether a key is one of the resources from the provided
   * resource IDs. Assumes that all of the given resource IDs are valid.
   */
  public static BackupKeyPredicate buildPredicateFromResourceIds(
      Context context, Collection<Integer> ids) {
    Set<String> keys = new HashSet<>();
    for (Integer id : ids) {
      keys.add(context.getString(id));
    }
    return in(keys);
  }

  /** Returns a predicate that returns true for any key. */
  public static BackupKeyPredicate alwaysTrue() {
    return new BackupKeyPredicate() {
      @Override
      public boolean shouldBeBackedUp(String key) {
        return true;
      }
    };
  }
}
