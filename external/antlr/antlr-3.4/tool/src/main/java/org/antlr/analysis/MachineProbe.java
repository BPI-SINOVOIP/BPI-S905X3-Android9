/*
 * [The "BSD license"]
 *  Copyright (c) 2010 Terence Parr
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.antlr.analysis;

import org.antlr.misc.IntSet;
import org.antlr.runtime.CommonToken;
import org.antlr.runtime.Token;
import org.antlr.tool.Grammar;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class MachineProbe {
	DFA dfa;

	public MachineProbe(DFA dfa) {
		this.dfa = dfa;
	}

	List<DFAState> getAnyDFAPathToTarget(DFAState targetState) {
		Set<DFAState> visited = new HashSet<DFAState>();
		return getAnyDFAPathToTarget(dfa.startState, targetState, visited);
	}

	public List<DFAState> getAnyDFAPathToTarget(DFAState startState,
			DFAState targetState, Set<DFAState> visited) {
		List<DFAState> dfaStates = new ArrayList<DFAState>();
		visited.add(startState);
		if (startState.equals(targetState)) {
			dfaStates.add(targetState);
			return dfaStates;
		}
		// for (Edge e : startState.edges) { // walk edges looking for valid
		// path
		for (int i = 0; i < startState.getNumberOfTransitions(); i++) {
			Transition e = startState.getTransition(i);
			if (!visited.contains(e.target)) {
				List<DFAState> path = getAnyDFAPathToTarget(
						(DFAState) e.target, targetState, visited);
				if (path != null) { // found path, we're done
					dfaStates.add(startState);
					dfaStates.addAll(path);
					return dfaStates;
				}
			}
		}
		return null;
	}

	/** Return a list of edge labels from start state to targetState. */
	public List<IntSet> getEdgeLabels(DFAState targetState) {
		List<DFAState> dfaStates = getAnyDFAPathToTarget(targetState);
		List<IntSet> labels = new ArrayList<IntSet>();
		for (int i = 0; i < dfaStates.size() - 1; i++) {
			DFAState d = dfaStates.get(i);
			DFAState nextState = dfaStates.get(i + 1);
			// walk looking for edge whose target is next dfa state
			for (int j = 0; j < d.getNumberOfTransitions(); j++) {
				Transition e = d.getTransition(j);
				if (e.target.stateNumber == nextState.stateNumber) {
					labels.add(e.label.getSet());
				}
			}
		}
		return labels;
	}

	/**
	 * Given List<IntSet>, return a String with a useful representation of the
	 * associated input string. One could show something different for lexers
	 * and parsers, for example.
	 */
	public String getInputSequenceDisplay(Grammar g, List<IntSet> labels) {
		List<String> tokens = new ArrayList<String>();
		for (IntSet label : labels)
			tokens.add(label.toString(g));
		return tokens.toString();
	}

	/**
	 * Given an alternative associated with a DFA state, return the list of
	 * tokens (from grammar) associated with path through NFA following the
	 * labels sequence. The nfaStates gives the set of NFA states associated
	 * with alt that take us from start to stop. One of the NFA states in
	 * nfaStates[i] will have an edge intersecting with labels[i].
	 */
	public List<Token> getGrammarLocationsForInputSequence(
			List<Set<NFAState>> nfaStates, List<IntSet> labels) {
		List<Token> tokens = new ArrayList<Token>();
		for (int i = 0; i < nfaStates.size() - 1; i++) {
			Set<NFAState> cur = nfaStates.get(i);
			Set<NFAState> next = nfaStates.get(i + 1);
			IntSet label = labels.get(i);
			// find NFA state with edge whose label matches labels[i]
			nfaConfigLoop: 
			
			for (NFAState p : cur) {
				// walk p's transitions, looking for label
				for (int j = 0; j < p.getNumberOfTransitions(); j++) {
					Transition t = p.transition(j);
					if (!t.isEpsilon() && !t.label.getSet().and(label).isNil()
							&& next.contains(t.target)) {
						if (p.associatedASTNode != null) {
							Token oldtoken = p.associatedASTNode.token;
							CommonToken token = new CommonToken(oldtoken
									.getType(), oldtoken.getText());
							token.setLine(oldtoken.getLine());
							token.setCharPositionInLine(oldtoken.getCharPositionInLine());
							tokens.add(token);
							break nfaConfigLoop; // found path, move to next
													// NFAState set
						}
					}
				}
			}
		}
		return tokens;
	}

	// /** Used to find paths through syntactically ambiguous DFA. If we've
	// * seen statement number before, what did we learn?
	// */
	// protected Map<Integer, Integer> stateReachable;
	//
	// public Map<DFAState, Set<DFAState>> getReachSets(Collection<DFAState>
	// targets) {
	// Map<DFAState, Set<DFAState>> reaches = new HashMap<DFAState,
	// Set<DFAState>>();
	// // targets can reach themselves
	// for (final DFAState d : targets) {
	// reaches.put(d,new HashSet<DFAState>() {{add(d);}});
	// }
	//
	// boolean changed = true;
	// while ( changed ) {
	// changed = false;
	// for (DFAState d : dfa.states.values()) {
	// if ( d.getNumberOfEdges()==0 ) continue;
	// Set<DFAState> r = reaches.get(d);
	// if ( r==null ) {
	// r = new HashSet<DFAState>();
	// reaches.put(d, r);
	// }
	// int before = r.size();
	// // add all reaches from all edge targets
	// for (Edge e : d.edges) {
	// //if ( targets.contains(e.target) ) r.add(e.target);
	// r.addAll( reaches.get(e.target) );
	// }
	// int after = r.size();
	// if ( after>before) changed = true;
	// }
	// }
	// return reaches;
	// }

}
