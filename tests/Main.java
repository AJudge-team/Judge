class Main {
    public static void main(String[] agrs) {
        int[] arr = new int[1000000];
        arr[0] = arr[1] = 1;
        for(int j=0;j<=1000;j++){
            for(int i = 2 ; i < 1000000 ; i++) {
                arr[i] = arr[i-1] + arr[i-2];
            }
        }

    }
}
