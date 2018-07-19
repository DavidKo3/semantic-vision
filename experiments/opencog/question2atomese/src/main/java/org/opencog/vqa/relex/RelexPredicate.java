package org.opencog.vqa.relex;

import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;

public class RelexPredicate implements Comparable<RelexPredicate> {
    private final String name;
    private final List<RelexArgument> arguments;

    public RelexPredicate(String name, RelexArgument firstArg, RelexArgument secondArg) {
        this.name = name;
        this.arguments = List.of(firstArg, secondArg);
        firstArg.addRelation(this);
        secondArg.addRelation(this);
    }

    public String getName() {
        return name;
    }
    
    public List<RelexArgument> getArguments() {
        return Collections.unmodifiableList(arguments);
    }
    
    @Override
    public int compareTo(RelexPredicate other) {
        if (getNumberOfArgumentUsages() != other.getNumberOfArgumentUsages()) {
            return getNumberOfArgumentUsages() - other.getNumberOfArgumentUsages();
        }
        return name.compareTo(other.name);
    }

    private int getNumberOfArgumentUsages() {
        return arguments.stream().collect(Collectors.summingInt(RelexArgument::getNumberOfUsages));
    }

    public String toFormula() {
        return name + "(" + arguments.stream().map(fn -> fn.getVariableName()).collect(Collectors.joining(", "))
                + ")";
    }

    public String toGroundedFormula() {
        return name + "(" + arguments.stream().map(fn -> fn.getName()).collect(Collectors.joining(", "))
                + ")";
    }

    public String toShortFormula() {
        return name + "()";
    }

    public String toAtomeseFormula() {
        if (name.equals("_predadj")) {
            String object = arguments.get(0).getVariableName();
            String firstPredicate = arguments.get(0).getName();
            String secondPredicate = arguments.get(1).getName();
            return String.format("(AndLink " +
                    "(InheritanceLink (VariableNode \"$%1$s\") (ConceptNode \"BoundingBox\"))" +
                    "(EvaluationLink (GroundedPredicateNode \"py:runNeuralNetwork\") (ListLink (VariableNode \"$%1$s\") (ConceptNode \"%2$s\")) )" +
                    "(EvaluationLink (GroundedPredicateNode \"py:runNeuralNetwork\") (ListLink (VariableNode \"$%1$s\") (ConceptNode \"%3$s\")) )" +
                    ")", object, firstPredicate, secondPredicate);
        }
        return "";   
    }
    
    @Override
    public String toString() {
        return name + "(" + arguments.stream().map(fn -> fn.toString()).collect(Collectors.joining(", ")) + ")";
    }
}