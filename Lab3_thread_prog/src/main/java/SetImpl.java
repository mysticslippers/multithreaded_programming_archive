import java.util.concurrent.atomic.AtomicMarkableReference;

public class SetImpl<T extends Comparable<T>> implements Set<T> {

    private final Node<T> tail;
    private final Node<T> head;

    public SetImpl() {
        tail = new Node<>(null, null);
        head = new Node<>(null, tail);
    }

    private static final class Node<T extends Comparable<T>> {
        final T value;
        final AtomicMarkableReference<Node<T>> next;

        Node(T value, Node<T> next) {
            this.value = value;
            this.next = new AtomicMarkableReference<>(next, false);
        }
    }

    private static final class Window<T extends Comparable<T>> {
        final Node<T> pred;
        final Node<T> curr;

        Window(Node<T> pred, Node<T> curr) {
            this.pred = pred;
            this.curr = curr;
        }
    }

    private Window<T> find(T value) {
        retry:
        while (true) {
            Node<T> pred = head;
            Node<T> curr = pred.next.getReference();

            while (true) {
                boolean[] marked = {false};
                Node<T> succ = curr.next.get(marked);

                while (marked[0]) {
                    if (!pred.next.compareAndSet(curr, succ, false, false)) {
                        continue retry;
                    }

                    curr = succ;
                    succ = curr.next.get(marked);
                }

                if (curr == tail || curr.value.compareTo(value) >= 0) {
                    return new Window<>(pred, curr);
                }

                pred = curr;
                curr = succ;
            }
        }
    }

    @Override
    public boolean add(T value) {
        while (true) {
            Window<T> window = find(value);
            Node<T> pred = window.pred;
            Node<T> curr = window.curr;

            if (curr != tail && curr.value.compareTo(value) == 0) {
                return false;
            }

            Node<T> newNode = new Node<>(value, curr);
            if (pred.next.compareAndSet(curr, newNode, false, false)) {
                return true;
            }
        }
    }

    @Override
    public boolean remove(T value) {
        while (true) {
            Window<T> window = find(value);
            Node<T> pred = window.pred;
            Node<T> curr = window.curr;

            if (curr == tail || curr.value.compareTo(value) != 0) {
                return false;
            }

            Node<T> succ = curr.next.getReference();

            if (!curr.next.compareAndSet(succ, succ, false, true)) {
                continue;
            }

            pred.next.compareAndSet(curr, succ, false, false);
            return true;
        }
    }

    @Override
    public boolean contains(T value) {
        Node<T> curr = head.next.getReference();

        while (curr != tail && curr.value.compareTo(value) < 0) {
            curr = curr.next.getReference();
        }

        if (curr == tail) {
            return false;
        }

        boolean[] marked = {false};
        curr.next.get(marked);

        return curr.value.compareTo(value) == 0 && !marked[0];
    }

    @Override
    public boolean isEmpty() {
        while (true) {
            Node<T> first = head.next.getReference();

            if (first == tail) {
                return true;
            }

            boolean[] marked = {false};
            Node<T> next = first.next.get(marked);

            if (!marked[0]) {
                return false;
            }

            head.next.compareAndSet(first, next, false, false);
        }
    }
}
