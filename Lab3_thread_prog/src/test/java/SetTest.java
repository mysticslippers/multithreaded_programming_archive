import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;

import static org.junit.Assert.*;

public class SetTest {

    @Test
    public void addSingleElement() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.add(10));
        assertTrue(set.contains(10));
        assertFalse(set.isEmpty());
    }

    @Test
    public void addDuplicateElement() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.add(10));
        assertFalse(set.add(10));
        assertTrue(set.contains(10));
    }

    @Test
    public void removeExistingElement() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.add(10));
        assertTrue(set.remove(10));
        assertFalse(set.contains(10));
        assertTrue(set.isEmpty());
    }

    @Test
    public void removeMissingElement() {
        Set<Integer> set = new SetImpl<>();

        assertFalse(set.remove(10));
        assertTrue(set.isEmpty());
    }

    @Test
    public void containsForMissingElement() {
        Set<Integer> set = new SetImpl<>();

        set.add(1);
        set.add(3);
        set.add(5);

        assertFalse(set.contains(2));
        assertFalse(set.contains(4));
        assertFalse(set.contains(6));
    }

    @Test
    public void isEmptyForNewSet() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.isEmpty());
    }

    @Test
    public void isEmptyAfterAddAndRemove() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.add(1));
        assertFalse(set.isEmpty());

        assertTrue(set.remove(1));
        assertTrue(set.isEmpty());
    }

    @Test
    public void severalElementsWorkCorrectly() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.add(5));
        assertTrue(set.add(1));
        assertTrue(set.add(3));

        assertTrue(set.contains(1));
        assertTrue(set.contains(3));
        assertTrue(set.contains(5));

        assertTrue(set.remove(3));
        assertFalse(set.contains(3));

        assertTrue(set.contains(1));
        assertTrue(set.contains(5));
        assertFalse(set.isEmpty());
    }

    @Test
    public void removeElementTwice() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.add(42));
        assertTrue(set.remove(42));
        assertFalse(set.remove(42));
        assertFalse(set.contains(42));
    }

    @Test
    public void addAfterRemove() {
        Set<Integer> set = new SetImpl<>();

        assertTrue(set.add(7));
        assertTrue(set.remove(7));
        assertTrue(set.add(7));
        assertTrue(set.contains(7));
    }

    @Test
    public void concurrentAddSameValueOnlyOneSucceeds() throws InterruptedException {
        final Set<Integer> set = new SetImpl<>();
        final int threads = 8;

        CountDownLatch ready = new CountDownLatch(threads);
        CountDownLatch start = new CountDownLatch(1);
        CountDownLatch done = new CountDownLatch(threads);

        List<BooleanHolder> results = new ArrayList<>();

        for (int i = 0; i < threads; i++) {
            BooleanHolder holder = new BooleanHolder();
            results.add(holder);

            Thread thread = new Thread(() -> {
                ready.countDown();
                await(start);
                holder.value = set.add(100);
                done.countDown();
            });
            thread.start();
        }

        ready.await();
        start.countDown();
        done.await();

        int successCount = 0;
        for (BooleanHolder holder : results) {
            if (holder.value) {
                successCount++;
            }
        }

        assertEquals(1, successCount);
        assertTrue(set.contains(100));
    }

    @Test
    public void concurrentAddDifferentValuesAllPresent() throws InterruptedException {
        final Set<Integer> set = new SetImpl<>();
        final int threads = 10;

        CountDownLatch ready = new CountDownLatch(threads);
        CountDownLatch start = new CountDownLatch(1);
        CountDownLatch done = new CountDownLatch(threads);

        for (int i = 0; i < threads; i++) {
            final int value = i;
            Thread thread = new Thread(() -> {
                ready.countDown();
                await(start);
                set.add(value);
                done.countDown();
            });
            thread.start();
        }

        ready.await();
        start.countDown();
        done.await();

        for (int i = 0; i < threads; i++) {
            assertTrue(set.contains(i));
        }

        assertFalse(set.isEmpty());
    }

    @Test
    public void concurrentRemoveSameValueOnlyOneSucceeds() throws InterruptedException {
        final Set<Integer> set = new SetImpl<>();
        final int threads = 8;

        assertTrue(set.add(55));

        CountDownLatch ready = new CountDownLatch(threads);
        CountDownLatch start = new CountDownLatch(1);
        CountDownLatch done = new CountDownLatch(threads);

        List<BooleanHolder> results = new ArrayList<>();

        for (int i = 0; i < threads; i++) {
            BooleanHolder holder = new BooleanHolder();
            results.add(holder);

            Thread thread = new Thread(() -> {
                ready.countDown();
                await(start);
                holder.value = set.remove(55);
                done.countDown();
            });
            thread.start();
        }

        ready.await();
        start.countDown();
        done.await();

        int successCount = 0;
        for (BooleanHolder holder : results) {
            if (holder.value) {
                successCount++;
            }
        }

        assertEquals(1, successCount);
        assertFalse(set.contains(55));
    }

    @Test
    public void concurrentAddAndRemoveDifferentValues() throws InterruptedException {
        final Set<Integer> set = new SetImpl<>();
        final int threads = 6;

        for (int i = 0; i < threads; i++) {
            assertTrue(set.add(i));
        }

        CountDownLatch ready = new CountDownLatch(threads);
        CountDownLatch start = new CountDownLatch(1);
        CountDownLatch done = new CountDownLatch(threads);

        for (int i = 0; i < threads; i++) {
            final int value = i;
            Thread thread = new Thread(() -> {
                ready.countDown();
                await(start);
                set.remove(value);
                done.countDown();
            });
            thread.start();
        }

        ready.await();
        start.countDown();
        done.await();

        for (int i = 0; i < threads; i++) {
            assertFalse(set.contains(i));
        }

        assertTrue(set.isEmpty());
    }

    private static void await(CountDownLatch latch) {
        try {
            latch.await();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            fail("Thread was interrupted");
        }
    }

    private static final class BooleanHolder {
        volatile boolean value;
    }
}
