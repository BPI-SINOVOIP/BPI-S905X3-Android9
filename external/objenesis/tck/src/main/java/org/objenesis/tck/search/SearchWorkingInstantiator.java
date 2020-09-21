/**
 * Copyright 2006-2017 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.objenesis.tck.search;

import org.objenesis.instantiator.ObjectInstantiator;
import org.objenesis.strategy.PlatformDescription;
import org.objenesis.tck.candidates.SerializableNoConstructor;

import java.io.Serializable;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.Iterator;
import java.util.SortedSet;

/**
 * This class will try every available instantiator on the platform to see which works.
 *
 * @author Henri Tremblay
 */
public class SearchWorkingInstantiator implements Serializable { // implements Serializable just for the test

    private SearchWorkingInstantiatorListener listener;

    public static void main(String[] args) throws Exception {
        System.out.println();
        System.out.println(PlatformDescription.describePlatform());
        System.out.println();

        SearchWorkingInstantiator searchWorkingInstantiator = new SearchWorkingInstantiator(new SystemOutListener());
        searchWorkingInstantiator.searchForInstantiator(SerializableNoConstructor.class);
    }

    public SearchWorkingInstantiator(SearchWorkingInstantiatorListener listener) {
        this.listener = listener;
    }

    public void searchForInstantiator(Class<?> toInstantiate) {
        SortedSet<String> classes = ClassEnumerator.getClassesForPackage(ObjectInstantiator.class.getPackage());

        for (Iterator<String> it = classes.iterator(); it.hasNext();) {
            String className = it.next();

            // Skip if inner class of isn't named like a instantiator
            if(className.contains("$") || !className.endsWith("Instantiator")) {
               continue;
            }

           Class<?> c = null;
           try {
              c = Class.forName(className);
           }
           catch(Exception e) {
              listener.instantiatorUnsupported(c, e);
              continue;
           }

           if(c.isInterface() || !ObjectInstantiator.class.isAssignableFrom(c)) {
                continue;
           }

            Constructor<?> constructor;
            try {
                constructor = c.getConstructor(Class.class);
            } catch (NoSuchMethodException e) {
                throw new RuntimeException(e);
            }

            try {
                ObjectInstantiator<?> instantiator =
                        (ObjectInstantiator<?>) constructor.newInstance(toInstantiate);
                instantiator.newInstance();
                listener.instantiatorSupported(c);
            }
            catch(Exception e) {
                Throwable t = (e instanceof InvocationTargetException) ? e.getCause() : e;
                listener.instantiatorUnsupported(c, t);
            }
        }
    }
}
