public class Quarter {
    public static int quarterOf(int month){
        if(month<=3){
            return 1;
        }else if(month<=6){
            return 2;
        }else if(month<=9){
            return 3;
        }else{
            return 4;
        }
    }

    public static void main(String [] args){
        System.out.println(quarterOf(3));
        System.out.println(quarterOf(8));
        System.out.println(quarterOf(11));
    }
}
